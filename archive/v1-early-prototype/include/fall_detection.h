#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <Arduino.h>
#include "sensor_vitals.h"
#include "sensor_imu.h"

extern std::queue<IMUData> accelDataQueue;

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
static constexpr uint32_t GATE0_DISARM_WINDOW_MS = 3000;

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

// ══════════════════════════════════════════════════════════════
// FALL DETECTION LOGIC USING IMU DATA
// ══════════════════════════════════════════════════════════════

// ── Fall Detection Logic Using IMU Data ───────────────────────
bool isFalling();
void evaluateFall();

#endif // FALL_DETECTION_H