#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <Arduino.h>
#include "sensor_vitals.h"
#include "sensor_imu.h"
#include "buttons.h"

extern IMURingBuffer<IMUData, 60> accelDataQueue;

// ══════════════════════════════════════════════════════════════
// GATE 0 — ON-BODY CONTACT VERIFICATION
// ══════════════════════════════════════════════════════════════
//
// Purpose:
//   Arms fall detection only when the device is confirmed to be
//   worn on a live human body. Prevents false positives from the
//   device sitting on a table, being carried in a bag, or being
//   thrown/dropped without a wearer.
//
// Arming logic (all conditions must hold continuously):
//   - bioHubReady == true        (sensor hardware is online)
//   - body.status == 3           (MAX30102 reports finger/skin contact)
//   - body.confidence >= threshold (signal quality is sufficient)
//   - For a sustained window of GATE0_ARM_WINDOW_MS
//
// Disarming logic:
//   - Any arming condition fails continuously for GATE0_DISARM_WINDOW_MS
//   - Disarming is BLOCKED once a fall event pipeline has started,
//     so contact loss mid-fall does not abort detection.
//
// Sensor failure handling:
//   - A single bad reading does NOT disarm. The sensor must be
//     continuously failed for the full GATE0_DISARM_WINDOW_MS
//     before Gate 0 disarms. This matches the arming hysteresis.
//
// ── Gate 0 Tuning Constants ───────────────────────────────────

// How long on-body signal must be sustained before arming (ms).
// 3 s gives enough time to confirm a genuine wear event and
// reject brief accidental contact or sensor settling noise.
static constexpr uint32_t GATE0_ARM_WINDOW_MS = 3000;

// How long off-body / sensor failure must persist before disarming (ms).
// Matches the arm window so the hysteresis is symmetric — a brief
// adjustment of the band or momentary signal dropout won't disarm.
static constexpr uint32_t GATE0_DISARM_WINDOW_MS = 10000;

// Minimum bio hub confidence score (0–100) to count as valid contact.
// Below this the optical signal is too noisy to trust, even if the
// status byte reports skin contact.
static constexpr uint8_t GATE0_MIN_CONFIDENCE = 50;

// ── Gate 0: Notify that a fall event has started ──────────────
// Called by Stage 1 of the fall pipeline the moment pre-impact
// motion is confirmed. Blocks disarming for the duration.
void gate0_notifyFallEventStarted();

// ── Gate 0: Notify that a fall event has resolved ─────────────
// Called when the fall pipeline fully resolves — either an alert
// was sent, the user self-cancelled, or the event timed out.
// Re-enables normal arming/disarming evaluation.
void gate0_notifyFallEventResolved();

// ── Gate 0: Main update — call from the task scheduler ────────
// Expected call rate: same cadence as readBioHub() (every 200 ms)
// so that every new bio hub reading is evaluated promptly.
void updateGate0();

// ── Gate 0: Public status queries ─────────────────────────────

// Returns true when fall detection is armed and the pipeline
// stages (1–5) are permitted to run.
bool gate0_isArmed();

// Returns true when the device is currently reading valid on-body
// contact (irrespective of armed state). Useful for the display
// to show a "NO CONTACT" warning when disarmed.
bool gate0_isOnBody();

// Returns the number of milliseconds remaining until Gate 0 arms,
// or 0 if already armed. Useful for a progress indicator on the
// display during the initial wear-on window.
uint32_t gate0_msUntilArmed();

uint32_t gate0_msUntilDisarmed();

// ══════════════════════════════════════════════════════════════
// FALL DETECTION LOGIC USING IMU DATA
// ══════════════════════════════════════════════════════════════

// ── Fall Detection Logic Using IMU Data ───────────────────────
bool isFalling();
void evaluateFall();

// ══════════════════════════════════════════════════════════════
// FALL DETECTION PIPELINE — STATE MACHINE (Stages 4 & 5)
// ══════════════════════════════════════════════════════════════
//
// Pipeline stages:
//   IDLE                  — armed and watching for a fall pattern
//   STAGE4_VERIFYING_REST — fall pattern detected; verifying that
//                           the wearer is now still (filters out
//                           false positives like violent arm jerks
//                           where motion continues immediately)
//   STAGE5_COUNTDOWN      — rest verified; countdown to alert with
//                           user opportunity to self-cancel via
//                           panic button short press
//   ALERTING              — alert fired; full alarm is active
//                           and will only stop on a long button hold
//
// Stage 4: rest verification window (ms). The maximum time the
// wearer has to settle into stillness after impact. If they never
// reach sustained rest within this window, the event is rejected.
// 3 s strikes a balance between rejecting false positives and not
// keeping a real-fall victim waiting too long.
static constexpr uint32_t STAGE4_REST_WINDOW_MS = 3000;

// Stage 4: how long sustained stillness must persist before Stage 4
// passes. The wearer can be in motion for the early portion of the
// 3-second window (natural post-impact bleed-off of the crutch
// tip's angular momentum) and still pass, as long as the final
// stretch is genuinely still. 1 second is short enough to feel
// responsive and long enough to reject brief still-points within
// continuous wild motion.
static constexpr uint32_t STAGE4_REQUIRED_STILLNESS_MS = 1000;

// Stage 4: motion thresholds. At true rest the accelerometer reads
// pure gravity ≈ 1 g (4096 LSB at ±8 g range). The band below
// allows for natural post-fall movements — the wearer breathing,
// slight shifts of weight, the crutch settling against the body —
// without admitting continued purposeful motion.
//
// The previous values (12000–22000 LSB ≈ 2.9–5.4 g) were centered
// far above gravity and rejected all real rest. Corrected here.
static constexpr float STAGE4_ACCEL_MAG_MIN = 2900.0f;   // ~0.71 g
static constexpr float STAGE4_ACCEL_MAG_MAX = 5300.0f;   // ~1.29 g

// Gyro max for "rest". Loosened from a strict 91 °/s because the
// crutch tip's angular momentum bleeds off over 200–400 ms after
// impact even when the wearer is stationary. 2500 LSB ≈ 152 °/s
// tolerates that bleed-off but excludes any sustained rotation.
static constexpr float STAGE4_GYRO_MAG_MAX  = 2500.0f;

// Stage 5: countdown duration (ms). Literature recommends 15–30 s.
// 20 s gives the wearer time to recognize the alert, locate the
// button, and cancel if they are fine; while still ensuring help
// arrives quickly in a true emergency.
static constexpr uint32_t STAGE5_COUNTDOWN_MS = 20000;

// Pipeline stage enumeration.
enum FallStage {
    FALL_IDLE,
    FALL_STAGE4_VERIFYING_REST,
    FALL_STAGE5_COUNTDOWN,
    FALL_ALERTING
};

// ── Public status queries for UI / telemetry ──────────────────

// Returns the current pipeline stage.
FallStage getFallStage();

// Returns ms remaining in the current Stage 5 countdown, or 0 if
// not in Stage 5. The display uses this to show the countdown.
uint32_t fallCountdownRemainingMs();

// Called by the alarm/button code to cancel a Stage 5 countdown
// when the user presses the panic button briefly. Returns true if
// a cancel was actually performed (so the caller knows to suppress
// the normal panic-alarm trigger for this press).
bool fallStage5_userCancel();

#endif // FALL_DETECTION_H