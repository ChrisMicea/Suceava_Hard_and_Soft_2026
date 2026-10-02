#include "fall_detection.h"
#include "alarm_buzzer.h"
#include "BLE_connection.h"

// ── Gate 0 Internal State ─────────────────────────────────────
namespace Gate0
{
    static bool     armed              = false;
    static uint32_t onBodySince        = 0;
    static uint32_t offBodySince       = 0;
    static bool     fallEventInProgress = false;

    // ── Internal: are the current bio hub readings on-body? ──
    static bool _isOnBodyNow()
    {
        if (!bioHubReady)                          return false;
        if (body.status != 3)                      return false;
        if (body.confidence < GATE0_MIN_CONFIDENCE) return false;
        return true;
    }
} // namespace Gate0

// ── Gate 0: Fall event lifecycle notifications ────────────────

void gate0_notifyFallEventStarted()
{
    Gate0::fallEventInProgress = true;
}

void gate0_notifyFallEventResolved()
{
    Gate0::fallEventInProgress = false;
    // Re-seed off-body timer so the full disarm window must elapse
    // fresh after the event, not counting time accrued during it.
    Gate0::offBodySince = millis();
}

// ── Gate 0: Main update (call at same rate as readBioHub) ─────
void updateGate0()
{
    uint32_t now    = millis();
    bool onBodyOtherCheck = Gate0::_isOnBodyNow();
    bool onBody = isTouchPressed();
    if (onBody || onBodyOtherCheck)
    {
        Gate0::offBodySince = 0;

        if (!Gate0::armed)
        {
            if (Gate0::onBodySince == 0)
            {
                Serial.println("[Gate0] On-body contact detected — starting arming timer.");
                Gate0::onBodySince = now;
            }
            if (now - Gate0::onBodySince >= GATE0_ARM_WINDOW_MS)
            {
                Gate0::armed = true;
                Serial.println("[Gate0] ARMED — sustained on-body contact confirmed.");
            }
        }
    }
    else
    {
        Gate0::onBodySince = 0;

        if (Gate0::armed)
        {
            if (Gate0::fallEventInProgress) return;

            if (Gate0::offBodySince == 0)
                Gate0::offBodySince = now;

            if (now - Gate0::offBodySince >= GATE0_DISARM_WINDOW_MS)
            {
                Gate0::armed        = false;
                Gate0::offBodySince = 0;
                Serial.println("[Gate0] DISARMED — sustained off-body / sensor failure.");
            }
        }
    }
}

// ── Gate 0: Public status queries ─────────────────────────────

bool gate0_isArmed()  { return Gate0::armed; }
bool gate0_isOnBody() { return Gate0::_isOnBodyNow(); }

uint32_t gate0_msUntilArmed()
{
    if (Gate0::armed)             return 0;
    if (Gate0::onBodySince == 0)  return GATE0_ARM_WINDOW_MS;
    uint32_t elapsed = millis() - Gate0::onBodySince;
    if (elapsed >= GATE0_ARM_WINDOW_MS) return 0;
    return GATE0_ARM_WINDOW_MS - elapsed;
}

uint32_t gate0_msUntilDisarmed()
{
    if (!Gate0::armed)            return 0;
    if (Gate0::offBodySince == 0) return GATE0_DISARM_WINDOW_MS;
    uint32_t elapsed = millis() - Gate0::offBodySince;
    if (elapsed >= GATE0_DISARM_WINDOW_MS) return 0;
    return GATE0_DISARM_WINDOW_MS - elapsed;
}

// ══════════════════════════════════════════════════════════════
// FALL DETECTION — STATE MACHINE
// ══════════════════════════════════════════════════════════════

static FallStage currentStage    = FALL_IDLE;
static uint32_t  stageEnteredAt  = 0;

// Tracks the most recent time (millis) we observed non-rest during
// Stage 4. Reset each time motion is seen; Stage 4 passes only when
// (now - stage4LastMotionMs) >= STAGE4_REQUIRED_STILLNESS_MS, i.e.
// the wearer has been sustainedly still for the required interval.
// This replaces the old "must be restful on the very first tick"
// semantics, which rejected legitimate falls during the natural
// 200–400 ms angular-momentum bleed-off at the crutch tip.
static uint32_t  stage4LastMotionMs = 0;

// Pre-event gravity vector, captured at the moment a fall pattern is
// detected (the oldest samples in the ring buffer, which represent
// the orientation BEFORE the descent + impact). Used at Stage 4 PASS
// to compare against the post-rest orientation. A real fall ends in
// a substantially different orientation; a "hard bounce" false alarm
// returns to essentially the same orientation as before.
//
// Stored as a normalized float vector for direct dot-product use.
static float preEventGx = 0.0f, preEventGy = 0.0f, preEventGz = -1.0f;
static bool  preEventBaselineValid = false;

// Minimum angle (degrees) the gravity vector must rotate between
// pre-event and post-rest for the event to count as a real fall.
// Empirically: hard-bounce false alarms show <5° change; real
// falls show 60°+ change. 30° sits comfortably in the gap.
static constexpr float ORIENTATION_CHANGE_MIN_DEG = 30.0f;

// ── Stage transition helpers ──────────────────────────────────
static void enterStage(FallStage next) {
    currentStage   = next;
    stageEnteredAt = millis();
    if (next == FALL_STAGE4_VERIFYING_REST) {
        // Assume in motion at entry — the impact sample itself
        // and the tail of the fall are anything but rest.
        stage4LastMotionMs = stageEnteredAt;
    }
}

static void rejectAndReset(const char* reason) {
    Serial.printf("[Fall] REJECTED in %s — %s\n",
                  currentStage == FALL_STAGE4_VERIFYING_REST ? "Stage 4" : "pipeline",
                  reason);
    enterStage(FALL_IDLE);
    gate0_notifyFallEventResolved();

    // Clear the ring buffer so stale impact samples cannot immediately
    // re-trigger isFalling() on the very next evaluation tick.
    // O(1) — just resets head and count, no element loop.
    accelDataQueue.clear();

    // Invalidate the orientation baseline so the next pipeline cycle
    // starts fresh.
    preEventBaselineValid = false;
}

// ── Capture pre-event gravity baseline ───────────────────────
// Called the moment a fall pattern is detected, BEFORE entering
// Stage 4. Averages the oldest BASELINE_SAMPLE_COUNT samples in
// the ring buffer — these are the samples furthest from the impact
// in time, representing the device orientation BEFORE the descent.
//
// The resulting vector is normalized so subsequent dot-product
// comparisons return a clean cosine of the angle.
static void capturePreEventBaseline() {
    constexpr uint8_t BASELINE_SAMPLE_COUNT = 10; // ~100 ms at 100 Hz

    uint8_t n = accelDataQueue.size();
    if (n < BASELINE_SAMPLE_COUNT) {
        // Not enough samples — fall back to the assumed-upright default.
        // Real falls always trigger after the buffer has filled, so this
        // is only an edge case during early operation.
        preEventBaselineValid = false;
        Serial.println("[Fall] Baseline capture failed — buffer too small.");
        return;
    }

    float sumX = 0, sumY = 0, sumZ = 0;
    for (uint8_t i = 0; i < BASELINE_SAMPLE_COUNT; i++) {
        IMUData d = accelDataQueue[i]; // index 0 = oldest
        sumX += (float)d.ax;
        sumY += (float)d.ay;
        sumZ += (float)d.az;
    }
    float avgX = sumX / BASELINE_SAMPLE_COUNT;
    float avgY = sumY / BASELINE_SAMPLE_COUNT;
    float avgZ = sumZ / BASELINE_SAMPLE_COUNT;

    float mag = sqrtf(avgX*avgX + avgY*avgY + avgZ*avgZ);
    if (mag < 1.0f) {
        // Numerical degenerate case — average was near-zero, shouldn't
        // happen with real sensor data but guard anyway.
        preEventBaselineValid = false;
        Serial.println("[Fall] Baseline capture failed — degenerate magnitude.");
        return;
    }

    preEventGx = avgX / mag;
    preEventGy = avgY / mag;
    preEventGz = avgZ / mag;
    preEventBaselineValid = true;

    Serial.printf("[Fall] Pre-event baseline captured: (%.2f, %.2f, %.2f)\n",
                  preEventGx, preEventGy, preEventGz);
}

// ── Compute orientation change from pre-event baseline ───────
// Averages the most recent BASELINE_SAMPLE_COUNT samples (the
// post-rest orientation) and returns the angle between that
// vector and the pre-event baseline, in degrees.
//
// Returns -1.0f if no baseline is available or computation fails.
static float computeOrientationChangeDeg() {
    if (!preEventBaselineValid) return -1.0f;

    constexpr uint8_t BASELINE_SAMPLE_COUNT = 10;
    uint8_t n = accelDataQueue.size();
    if (n < BASELINE_SAMPLE_COUNT) return -1.0f;

    // Average the most recent BASELINE_SAMPLE_COUNT samples.
    float sumX = 0, sumY = 0, sumZ = 0;
    uint8_t start = n - BASELINE_SAMPLE_COUNT;
    for (uint8_t i = start; i < n; i++) {
        IMUData d = accelDataQueue[i];
        sumX += (float)d.ax;
        sumY += (float)d.ay;
        sumZ += (float)d.az;
    }
    float avgX = sumX / BASELINE_SAMPLE_COUNT;
    float avgY = sumY / BASELINE_SAMPLE_COUNT;
    float avgZ = sumZ / BASELINE_SAMPLE_COUNT;

    float mag = sqrtf(avgX*avgX + avgY*avgY + avgZ*avgZ);
    if (mag < 1.0f) return -1.0f;

    float nx = avgX / mag;
    float ny = avgY / mag;
    float nz = avgZ / mag;

    // Dot product of two unit vectors is the cosine of the angle.
    float cosTheta = preEventGx * nx + preEventGy * ny + preEventGz * nz;
    if (cosTheta >  1.0f) cosTheta =  1.0f;
    if (cosTheta < -1.0f) cosTheta = -1.0f;

    return acosf(cosTheta) * 57.2957795f; // rad → deg
}

// ── Internal helper: is the wearer currently still? ──────────
//
// Examines only the most-recent REST_SAMPLE_COUNT samples.
// After a genuine fall the wearer is largely motionless on the
// ground; after a violent arm-jerk motion continues immediately.
// This is the key discriminating signal for Stage 4.
//
// With the ring buffer, reading the tail is a direct indexed
// access — no copy, no pop loop, no temporary allocation.
// The oldest sample is index 0; index size()-1 is the newest.
static bool isCurrentlyRestful() {
    constexpr uint8_t REST_SAMPLE_COUNT = 10; // ~100 ms at 100 Hz

    uint8_t n = accelDataQueue.size();
    if (n < REST_SAMPLE_COUNT) {
        return false; // Not enough fresh data — fail safe.
    }

    // Walk the REST_SAMPLE_COUNT newest samples via direct index.
    // start = first index of the tail window (oldest of the tail).
    uint8_t start = n - REST_SAMPLE_COUNT;

    for (uint8_t i = start; i < n; i++) {
        IMUData d = accelDataQueue[i]; // O(1), no copy of full buffer

        float accelMag = sqrtf((float)((int32_t)d.ax * d.ax +
                                       (int32_t)d.ay * d.ay +
                                       (int32_t)d.az * d.az));
        float gyroMag  = sqrtf((float)((int32_t)d.gx * d.gx +
                                       (int32_t)d.gy * d.gy +
                                       (int32_t)d.gz * d.gz));

        if (accelMag < STAGE4_ACCEL_MAG_MIN) return false; // too light — still moving
        if (accelMag > STAGE4_ACCEL_MAG_MAX) return false; // too heavy — still impacting
        if (gyroMag  > STAGE4_GYRO_MAG_MAX)  return false; // still rotating
    }

    return true;
}

// ── evaluateFall: called by the scheduler every 500 ms ────────
void evaluateFall() {
    uint32_t now = millis();

    switch (currentStage) {

    case FALL_IDLE:
        if (isFalling()) {
            Serial.println("[Fall] Pattern detected — entering Stage 4 (rest verification).");
            // Capture the pre-event orientation BEFORE entering Stage 4.
            // The oldest samples in the ring buffer represent the device
            // orientation before the descent began.
            capturePreEventBaseline();
            gate0_notifyFallEventStarted();
            enterStage(FALL_STAGE4_VERIFYING_REST);
        }
        break;

    case FALL_STAGE4_VERIFYING_REST: {
        uint32_t elapsedInStage = now - stageEnteredAt;
        bool restful = isCurrentlyRestful();

        if (!restful) {
            // Still in motion — reset the stillness clock. Real falls
            // need 200–400 ms of bleed-off before the crutch tip's
            // angular momentum dissipates, so we tolerate motion in
            // the early portion of the window.
            stage4LastMotionMs = now;

            if (elapsedInStage >= STAGE4_REST_WINDOW_MS) {
                // Full window elapsed and the wearer never settled.
                // This is the deliberate-crutch-wave rejection path:
                // continuous motion that never reaches rest is not a
                // fall. The wearer must either stop briefly within
                // 3 s or be rejected here.
                rejectAndReset("never reached sustained rest within window");
            } else {
                Serial.printf("[Fall] Stage 4 — in motion, %lums elapsed of %lums.\n",
                              (unsigned long)elapsedInStage,
                              (unsigned long)STAGE4_REST_WINDOW_MS);
            }
            break;
        }

        // Currently restful. Has stillness been sustained long enough?
        uint32_t restfulFor = now - stage4LastMotionMs;
        if (restfulFor >= STAGE4_REQUIRED_STILLNESS_MS) {
            // Final gate: did the device's orientation actually CHANGE?
            // A real fall leaves the crutch in a substantially different
            // orientation than before (lying flat, sideways, etc.). A
            // hard-bounce false alarm — slamming the tip on the ground —
            // returns to essentially the same orientation as before.
            //
            // Empirical separation from captured data:
            //   • Hard-bounce false alarms: ~1–5° change
            //   • Real falls:               ~60–100° change
            // 30° threshold sits cleanly in the gap.
            float orientChangeDeg = computeOrientationChangeDeg();
            if (orientChangeDeg < 0.0f) {
                // Couldn't compute — fail open (accept the event).
                // The baseline-capture failure case is rare and only
                // happens during early operation; safer to alarm than
                // to miss a real fall here.
                Serial.println("[Fall] Stage 4 PASSED — sustained rest verified "
                               "(orientation unavailable, accepting).");
                enterStage(FALL_STAGE5_COUNTDOWN);
            } else if (orientChangeDeg < ORIENTATION_CHANGE_MIN_DEG) {
                Serial.printf("[Fall] Stage 4 — orientation barely changed "
                              "(%.1f° < %.1f°). Likely hard-bounce, not a fall.\n",
                              orientChangeDeg, ORIENTATION_CHANGE_MIN_DEG);
                rejectAndReset("no orientation change — hard bounce");
            } else {
                Serial.printf("[Fall] Stage 4 PASSED — sustained rest %lums, "
                              "orientation changed %.1f°.\n",
                              (unsigned long)restfulFor, orientChangeDeg);
                enterStage(FALL_STAGE5_COUNTDOWN);
            }
        } else if (elapsedInStage >= STAGE4_REST_WINDOW_MS) {
            // Defensive: we reached the end of the window while
            // technically restful but not for long enough. This is
            // an unusual case (brief still-point in continuous
            // motion) — reject.
            rejectAndReset("stillness not sustained before window expired");
        } else {
            Serial.printf("[Fall] Stage 4 — restful for %lums, need %lums.\n",
                          (unsigned long)restfulFor,
                          (unsigned long)STAGE4_REQUIRED_STILLNESS_MS);
        }
        break;
    }

    case FALL_STAGE5_COUNTDOWN: {
        uint32_t elapsed = now - stageEnteredAt;
        if (elapsed >= STAGE5_COUNTDOWN_MS) {
            Serial.println("[Fall] Countdown expired — ALERTING.");
            enterStage(FALL_ALERTING);
            alarmActive          = true;
            lastBuzzerToggleTime = now;
            buzzerOn             = true;
            digitalWrite(BUZZER_PIN, HIGH);
            sendPanic(true);   // Notify phone: isFall = true
        } else {
            Serial.printf("[Fall] Countdown — %lus remaining.\n",
                          (unsigned long)((STAGE5_COUNTDOWN_MS - elapsed) / 1000));
        }
        break;
    }

    case FALL_ALERTING:
        if (!alarmActive) {
            Serial.println("[Fall] Alarm cleared — returning to IDLE.");
            enterStage(FALL_IDLE);
            gate0_notifyFallEventResolved();
            accelDataQueue.clear(); // O(1) reset
            preEventBaselineValid = false;
        }
        break;
    }
}

// ── Public status queries ─────────────────────────────────────

FallStage getFallStage() { return currentStage; }

uint32_t fallCountdownRemainingMs() {
    if (currentStage != FALL_STAGE5_COUNTDOWN) return 0;
    uint32_t elapsed = millis() - stageEnteredAt;
    if (elapsed >= STAGE5_COUNTDOWN_MS) return 0;
    return STAGE5_COUNTDOWN_MS - elapsed;
}

bool fallStage5_userCancel() {
    if (currentStage != FALL_STAGE5_COUNTDOWN) return false;
    Serial.println("[Fall] User cancelled countdown — returning to IDLE.");
    enterStage(FALL_IDLE);
    gate0_notifyFallEventResolved();
    accelDataQueue.clear(); // O(1) reset
    preEventBaselineValid = false;
    return true;
}

// ══════════════════════════════════════════════════════════════
// FALL DETECTION THRESHOLDS — TWO-MODE DETECTOR
// ══════════════════════════════════════════════════════════════
//
// All thresholds in raw MPU-6050 LSB units.
// Accel at ±8 g range:   4096 LSB/g
// Gyro  at ±2000 °/s:    16.4 LSB/(°/s)
//
// The detector now has TWO sensitivity modes selected by the
// touch sensor on the handle at evaluation time:
//
//   STRICT (no grip detected):
//     Used when the user is not currently gripping the handle.
//     This is when most false alarms occur — banging an unheld
//     crutch on the ground, setting it down hard, etc. We demand
//     strong kinematic evidence.
//
//     Within STRICT, two sub-paths handle different physics:
//
//       Path A — free-fall present:
//         Free-fall is itself a strong fall indicator. We only
//         need modest impact + rotation to confirm.
//         (impact > IMPACT_LENIENT) AND (gyro > GYRO_LENIENT)
//
//       Path B — no free-fall:
//         Without a free-fall dip we cannot distinguish a fall
//         from a hard knock on kinematics alone. We require
//         strong impact AND strong rotation.
//         (impact > IMPACT_STRICT) AND (gyro > GYRO_STRICT)
//
//   HELD (grip detected via touch sensor):
//     Used when the wearer is actively holding the handle.
//     Held-crutch falls show much milder tip kinematics because
//     the user's arm absorbs impact and partly controls the
//     descent. We use the lenient thresholds regardless of
//     free-fall, because held falls often lack a clear
//     free-fall dip.
//         (impact > IMPACT_LENIENT) AND (gyro > GYRO_LENIENT)
//
// In all paths, the descent must precede the impact in time
// (correctSequence), and Stage 4 then verifies that the wearer
// settles into stillness and that the device orientation
// actually changed (rejects bounces).
//
// Calibrated against captured events:
//
//                                minA   maxA    maxG  | held?
//   Dropped fall (line 469):     1072  38091   3408   |   no    → Path A ✓
//   Slam-fall (line 25):          782  14916   5092   |   no    → Path A ✓
//   Missed held fall (window):   3072   6536   2933   |  YES    → HELD  ✓
//   Hard-knock false alarm:      3247  26495   3309   |   no    → Path B → rejected
//                                                                 (gyro 3309 < strict 5740)

// ── Free-fall dip threshold ───────────────────────────────────
// ~0.73 g (3000 LSB). Used as the "Path A" gate in strict mode.
static constexpr float FREE_FALL_THRESHOLD = 3000.0f;

// ── Lenient thresholds (HELD or strict with free-fall) ────────
// Impact: ~1.39 g (5700 LSB). Above sustained-walking peaks but
// below the missed held fall's 1.60 g peak.
// Rotation: ~150 °/s (2460 LSB). Above typical walking gyro
// peaks but below the missed held fall's 179 °/s peak.
static constexpr float IMPACT_LENIENT = 5700.0f;
static constexpr float GYRO_LENIENT   = 2460.0f;

// ── Strict thresholds (no grip, no free-fall) ─────────────────
// These are the hardest path to pass — used only when the
// crutch is unheld and never enters free-fall. Hard-knock false
// alarms live here, so we require strong evidence.
// Impact: ~1.95 g (8000 LSB). Strongly above gait spikes.
// Rotation: ~350 °/s (5740 LSB). Above the 310 °/s peak observed
// in hard-knock false alarms; below the 486+ °/s seen in real
// non-held falls without free-fall.
static constexpr float IMPACT_STRICT = 8000.0f;
static constexpr float GYRO_STRICT   = 5740.0f;

// Maximum sample gap between descent start and impact spike.
// At 100 Hz, 50 samples = 500 ms — covers the longest realistic
// fall arc without admitting unrelated later spikes.
static constexpr int MAX_FALL_DURATION_SAMPLES = 50;

// ── isFalling: sliding-window pattern detector ────────────────
//
// Requires within the current 60-sample (600 ms) window:
//   • Impact spike       — accel mag > IMPACT_THRESHOLD
//   • Strong rotation    — gyro mag  > GYRO_THRESHOLD
//   • Correct sequence   — rotation/free-fall precedes impact
//
// Free-fall (FREE_FALL_THRESHOLD) is OPTIONAL — it improves the
// sequence anchor when present (dropped-crutch falls) but is not
// required (held-crutch falls show no free-fall dip).
//
// Why no strong-rotation OR-gate any more:
//   An earlier version OR'd free-fall with strong rotation. Hard-
//   knock false alarms had deep free-fall dips (the tip is forced
//   downward briefly during the knock) so they passed the OR-gate
//   and progressed to impact + impact. Raising the rotation
//   threshold and requiring it unconditionally is cleaner: the
//   gyro signature is what physically separates a tumble from a
//   localized hit, so we make it the primary gate.
//
// Stage 4 (rest verification) still catches any false positives
// from aggressive crutch waving — those scenarios show continued
// motion after the supposed "impact".
bool isFalling() {
    uint8_t n = accelDataQueue.size();

    // Need at least 30 samples (~300 ms) to capture a full fall arc.
    if (n < 30) return false;

    float minAccelMag = 999999.0f,  maxAccelMag = 0.0f;
    float maxGyroMag  = 0.0f;
    int   minAccelIdx = -1, maxAccelIdx = -1, maxGyroIdx = -1;

    for (uint8_t i = 0; i < n; i++) {
        IMUData d = accelDataQueue[i]; // direct O(1) access

        float accelMag = sqrtf((float)((int32_t)d.ax * d.ax +
                                       (int32_t)d.ay * d.ay +
                                       (int32_t)d.az * d.az));
        float gyroMag  = sqrtf((float)((int32_t)d.gx * d.gx +
                                       (int32_t)d.gy * d.gy +
                                       (int32_t)d.gz * d.gz));

        if (accelMag < minAccelMag) { minAccelMag = accelMag; minAccelIdx = i; }
        if (accelMag > maxAccelMag) { maxAccelMag = accelMag; maxAccelIdx = i; }
        if (gyroMag  > maxGyroMag)  { maxGyroMag  = gyroMag;  maxGyroIdx  = i; }
    }

    // ── Choose threshold path based on grip + free-fall ──────
    // See the comment block above the threshold constants for
    // the full rationale. Summary:
    //   HELD (touch active):   lenient impact + lenient gyro
    //   STRICT-A (free-fall):  lenient impact + lenient gyro
    //   STRICT-B (no free-fall): strict impact + strict gyro
    bool isHeld           = isTouchPressed();
    bool freeFallDetected = (minAccelMag < FREE_FALL_THRESHOLD);

    float impactThresh, gyroThresh;
    const char* pathName;
    if (isHeld) {
        impactThresh = IMPACT_LENIENT;
        gyroThresh   = GYRO_LENIENT;
        pathName     = "HELD";
    } else if (freeFallDetected) {
        impactThresh = IMPACT_LENIENT;
        gyroThresh   = GYRO_LENIENT;
        pathName     = "STRICT-A";  // unheld + free-fall
    } else {
        impactThresh = IMPACT_STRICT;
        gyroThresh   = GYRO_STRICT;
        pathName     = "STRICT-B";  // unheld, no free-fall
    }

    bool impactDetected   = (maxAccelMag > impactThresh);
    bool rotationDetected = (maxGyroMag  > gyroThresh);

    // Descent start: the earlier of the free-fall dip and the
    // rotation peak, whichever is present. With strong rotation
    // required, maxGyroIdx is always available as a fallback
    // anchor; minAccelIdx improves the timing when free-fall
    // is also observed.
    int descentStartIdx = maxGyroIdx;
    if (freeFallDetected && minAccelIdx >= 0 && minAccelIdx < descentStartIdx) {
        descentStartIdx = minAccelIdx;
    }

    bool correctSequence = (descentStartIdx >= 0) &&
                           (descentStartIdx < maxAccelIdx) &&
                           ((maxAccelIdx - descentStartIdx) <= MAX_FALL_DURATION_SAMPLES);

    Serial.printf(
        "[Fall] minA=%.0f@%d  maxA=%.0f@%d  maxG=%.0f@%d"
        "  | path=%s FF=%d IMP=%d ROT=%d SEQ=%d\n",
        minAccelMag, minAccelIdx,
        maxAccelMag, maxAccelIdx,
        maxGyroMag,  maxGyroIdx,
        pathName, freeFallDetected, impactDetected,
        rotationDetected, correctSequence);

    return impactDetected && rotationDetected && correctSequence;
}