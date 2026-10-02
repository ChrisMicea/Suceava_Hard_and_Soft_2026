#include "fall_detection.h"

// ── Gate 0 Internal State ─────────────────────────────────────
namespace Gate0
{
    // Current armed state of the fall detection pipeline.
    static bool armed = false;

    // Timestamp when the current on-body streak started (ms).
    // Reset to 0 whenever contact is lost.
    static uint32_t onBodySince = 0;

    // Timestamp when the current off-body / failure streak started (ms).
    // Reset to 0 whenever valid contact is restored.
    static uint32_t offBodySince = 0;

    // Set by the fall pipeline when a fall event is in progress.
    // While true, disarming is blocked regardless of sensor readings.
    static bool fallEventInProgress = false;

    // ── Internal: Evaluate whether current readings are on-body ──
    // Returns true only when the bio hub is online, status is finger-
    // detected (3), and confidence meets the minimum threshold.
    static bool _isOnBodyNow()
    {
        if (!bioHubReady)
            return false;
        if (body.status != 3)
            return false;
        if (body.confidence < GATE0_MIN_CONFIDENCE)
            return false;
        return true;
    }

} // namespace Gate0

// ── Gate 0: Notify that a fall event has started ──────────────
// Called by Stage 1 of the fall pipeline the moment pre-impact
// motion is confirmed. Blocks disarming for the duration.
void gate0_notifyFallEventStarted()
{
    Gate0::fallEventInProgress = true;
}

// ── Gate 0: Notify that a fall event has resolved ─────────────
// Called when the fall pipeline fully resolves — either an alert
// was sent, the user self-cancelled, or the event timed out.
// Re-enables normal arming/disarming evaluation.
void gate0_notifyFallEventResolved()
{
    Gate0::fallEventInProgress = false;

    // Re-seed the off-body timer from now so the full disarm window
    // must elapse fresh after the event, rather than using a stale
    // timestamp that may have accumulated during the event.
    Gate0::offBodySince = millis();
}

// ── Gate 0: Main update — call from the task scheduler ────────
// Expected call rate: same cadence as readBioHub() (every 200 ms)
// so that every new bio hub reading is evaluated promptly.
void updateGate0()
{
    uint32_t now = millis();
    bool onBody = Gate0::_isOnBodyNow();

    if (onBody)
    {
        // Valid contact: reset the off-body streak timer.
        Gate0::offBodySince = 0;

        if (!Gate0::armed)
        {
            // Start or continue accumulating the arming window.
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
        // If already armed, nothing to do — stay armed.
    }
    else
    {
        // No valid contact: reset the on-body streak timer.
        Gate0::onBodySince = 0;

        if (Gate0::armed)
        {
            // Disarming is blocked while a fall event is in progress.
            if (Gate0::fallEventInProgress)
                return;

            // Start or continue accumulating the disarm window.
            if (Gate0::offBodySince == 0)
            {
                Gate0::offBodySince = now;
            }

            if (now - Gate0::offBodySince >= GATE0_DISARM_WINDOW_MS)
            {
                Gate0::armed = false;
                Gate0::offBodySince = 0;
                Serial.println("[Gate0] DISARMED — sustained off-body / sensor failure.");
            }
        }
        // If already disarmed, nothing to do — stay disarmed.
    }
}

// ── Gate 0: Public status queries ─────────────────────────────

// Returns true when fall detection is armed and the pipeline
// stages (1–5) are permitted to run.
bool gate0_isArmed()
{
    return Gate0::armed;
}

// Returns true when the device is currently reading valid on-body
// contact (irrespective of armed state). Useful for the display
// to show a "NO CONTACT" warning when disarmed.
bool gate0_isOnBody()
{
    return Gate0::_isOnBodyNow();
}

// Returns the number of milliseconds remaining until Gate 0 arms,
// or 0 if already armed. Useful for a progress indicator on the
// display during the initial wear-on window.
uint32_t gate0_msUntilArmed()
{
    if (Gate0::armed)
        return 0;
    if (Gate0::onBodySince == 0)
        return GATE0_ARM_WINDOW_MS;
    uint32_t elapsed = millis() - Gate0::onBodySince;
    if (elapsed >= GATE0_ARM_WINDOW_MS)
        return 0;
    return GATE0_ARM_WINDOW_MS - elapsed;
}

// ══════════════════════════════════════════════════════════════
// FALL DETECTION LOGIC USING IMU DATA
// ══════════════════════════════════════════════════════════════

void evaluateFall() {
  if (isFalling()) {
    Serial.println("Fall Detected! YES");
    // Additional logic for fall response can be added here
  }
  else {
    Serial.println("No fall detected. NO");
  }
}

// --- FALL DETECTION THRESHOLDS ---
// Accel thresholds based on raw MPU-6050 LSB units at ±2g range (16384 LSB/g).
// Baseline 1g = ~16500 LSB.
//
// 1. Free Fall: weightless drop below ~0.3g (~5000 LSB). Empirically observed
//    dips reach as low as ~1900 LSB during real falls. A loose threshold of
//    5000 keeps sensitivity high while still rejecting normal motion (which
//    rarely dips below ~12000 LSB even during brisk arm swings).
static constexpr float FREE_FALL_THRESHOLD = 3000.0f;

// 2. Impact: violent spike above ~1.8g (~30000 LSB combined magnitude).
//    Real impacts on hard floors easily exceed 35000–45000 LSB.
static constexpr float IMPACT_THRESHOLD = 30000.0f;

// 3. Rotation: gyro magnitude during the descent must show body rotation,
//    not just translation. MPU-6050 at ±250°/s gives 131 LSB/(°/s).
//    Literature threshold of ~47°/s ≈ 6200 LSB. We require 8000 LSB (~61°/s)
//    to comfortably exceed normal arm swing rotation (~30–50°/s).
static constexpr float GYRO_THRESHOLD = 8000.0f;

// Maximum samples between free-fall dip and impact spike. At 100 Hz, a real
// fall arc completes in ~30–50 samples (300–500 ms). Allowing up to 50
// captures the full event without admitting unrelated spikes from later
// activity in the same window.
static constexpr int MAX_FALL_DURATION_SAMPLES = 50;

// Evaluates the sliding window queue to detect a sequential
// Free Fall + Rotation + Impact pattern.
bool isFalling() {
  // Need enough samples to capture a full fall arc (~300 ms = 30 samples at 100 Hz).
  if (accelDataQueue.size() < 30) {
    return false;
  }

  // Accel extremes
  float minAccelMag = 999999.0f;
  float maxAccelMag = 0.0f;
  int   minAccelIdx = -1;
  int   maxAccelIdx = -1;

  // Gyro peak
  float maxGyroMag = 0.0f;
  int   maxGyroIdx = -1;

  int currentIndex = 0;
  std::queue<IMUData> tempQueue = accelDataQueue;  // Copy to preserve original

  while (!tempQueue.empty()) {
    IMUData d = tempQueue.front();
    tempQueue.pop();

    // Accel magnitude — int32 intermediate is safe (max ~1.07e9 < 2.1e9 INT32_MAX).
    float accelMag = sqrtf((float)((int32_t)d.ax * d.ax +
                                   (int32_t)d.ay * d.ay +
                                   (int32_t)d.az * d.az));

    // Gyro magnitude — same int32 safety margin.
    float gyroMag  = sqrtf((float)((int32_t)d.gx * d.gx +
                                   (int32_t)d.gy * d.gy +
                                   (int32_t)d.gz * d.gz));

    if (accelMag < minAccelMag) { minAccelMag = accelMag; minAccelIdx = currentIndex; }
    if (accelMag > maxAccelMag) { maxAccelMag = accelMag; maxAccelIdx = currentIndex; }
    if (gyroMag  > maxGyroMag)  { maxGyroMag  = gyroMag;  maxGyroIdx  = currentIndex; }

    currentIndex++;
  }

  // --- PATTERN VALIDATION ---
  bool freeFallDetected = (minAccelMag < FREE_FALL_THRESHOLD);
  bool impactDetected   = (maxAccelMag > IMPACT_THRESHOLD);
  bool rotationDetected = (maxGyroMag  > GYRO_THRESHOLD);

  // Chronological: free fall comes BEFORE impact, separated by a realistic gap.
  bool correctSequence  = (minAccelIdx < maxAccelIdx) &&
                          ((maxAccelIdx - minAccelIdx) <= MAX_FALL_DURATION_SAMPLES);

  // Rotation must accompany the descent — peak gyro should occur at or after
  // the free-fall dip, and at or before the impact spike (with a small slack).
  bool rotationInSequence = (maxGyroIdx >= minAccelIdx - 2) &&
                            (maxGyroIdx <= maxAccelIdx + 2);

  Serial.printf("[Fall] minA=%.0f@%d  maxA=%.0f@%d  maxG=%.0f@%d  | FF=%d IMP=%d ROT=%d SEQ=%d ROTSEQ=%d\n",
                minAccelMag, minAccelIdx,
                maxAccelMag, maxAccelIdx,
                maxGyroMag,  maxGyroIdx,
                freeFallDetected, impactDetected, rotationDetected, correctSequence, rotationInSequence);   

  if ((freeFallDetected && impactDetected && correctSequence) && (rotationDetected && rotationInSequence)) {
    return true;
  }
  return false;
}