// #include "alarm_buzzer.h"
// #include "fall_detection.h"
// #include "buttons.h"
// #include "BLE_connection.h"

// // ── Alarm State Tracking Variables ────────────────────────────
// bool alarmActive = false;
// bool buzzerOn = false;
// unsigned long lastBuzzerToggleTime = 0;
// unsigned long buttonPressedStartTime = 0;
// bool trackingHoldTime = false;

// // ── Edge-detect state for short-press fall cancel ─────────────
// // We need to distinguish a "press" (rising edge) from "held".
// // Only a fresh press during Stage 5 cancels the countdown; a
// // continuously-held button does not.
// static bool prevPanicPressed = false;

// // ── Post-reset release lockout ────────────────────────────────
// // After a successful hold-reset the button is still physically
// // held down. Without this flag the very next tick sees
// // panicPressed==true and immediately re-arms the alarm.
// // The flag blocks all panic logic until the button is released.
// static bool waitingForRelease = false;

// // ── Handle Panic Button & Alarm Logic ─────────────────────────
// void updateAlarmState() {
//   unsigned long nowMs = millis();
//   bool panicPressed = isPanicButtonPressed();
//   bool freshPress = panicPressed && !prevPanicPressed;
//   prevPanicPressed = panicPressed;

//   // ── Release lockout: swallow all input until finger lifts ──
//   // Set after a successful hold-reset; cleared the moment the
//   // button is no longer pressed. Prevents the still-held button
//   // from immediately re-triggering the alarm on the next tick.
//   if (waitingForRelease) {
//     if (!panicPressed && !isInDebounce()) waitingForRelease = false;
//     return;
//   }

//   // ── Stage 5 cancel takes priority over normal panic logic ──
//   // If we're in the fall-detection countdown, a fresh press
//   // cancels the countdown and CONSUMES the press — it does not
//   // also trigger the manual panic alarm.
//   if (freshPress && fallStage5_userCancel()) {
//     return;
//   }

//   if (!alarmActive) {
//     if (panicPressed) {
//       alarmActive = true;
//       lastBuzzerToggleTime = nowMs;
//       buzzerOn = true;
//       digitalWrite(BUZZER_PIN, HIGH);
//       sendPanic(false);   // Notify phone: isFall = false (manual panic)
//     }
//   } 
//   else {
//     // Handle panic button hold to reset alarm
//     if (panicPressed) {
//       if (!trackingHoldTime) {
//         buttonPressedStartTime = nowMs;
//         trackingHoldTime = true;
//       } else if (nowMs - buttonPressedStartTime >= 3000) {
//         alarmActive = false;
//         buzzerOn = false;
//         digitalWrite(BUZZER_PIN, LOW);
//         trackingHoldTime = false;
//         waitingForRelease = true; // Block until the button is physically released
//       }
//     } 
//     else {
//       if (!isInDebounce()) {
//         trackingHoldTime = false;
//       }
//     }

//     // Toggle buzzer every second when alarm is active
//     if (alarmActive) {
//       if (nowMs - lastBuzzerToggleTime >= 1000) {
//         buzzerOn = !buzzerOn;
//         digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
//         lastBuzzerToggleTime = nowMs;
//       }
//     }
//   }
// }

// // ── Get alarm status ──────────────────────────────────────────
// bool isAlarmActive() {
//   return alarmActive;
// }

// // ── Get reset in progress status ──────────────────────────────
// bool isResettingAlarm() {
//   return trackingHoldTime;
// }

// // ── Initialize Alarm/Buzzer Hardware ──────────────────────────
// void initializeAlarmHardware() {
//   setupButtons();
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
// }

#include "alarm_buzzer.h"
#include "fall_detection.h"
#include "buttons.h"
#include "BLE_connection.h"

// ── Alarm State Tracking Variables ────────────────────────────
bool alarmActive = false;
bool buzzerOn = false;
unsigned long lastBuzzerToggleTime = 0;
unsigned long buttonPressedStartTime = 0;
bool trackingHoldTime = false;

// ── Edge-detect state for short-press fall cancel ─────────────
// We need to distinguish a "press" (rising edge) from "held".
// Only a fresh press during Stage 5 cancels the countdown; a
// continuously-held button does not.
static bool prevPanicPressed = false;

// ── Post-reset release lockout ────────────────────────────────
// After a successful hold-reset the button is still physically
// held down. Without this flag the very next tick sees
// panicPressed==true and immediately re-arms the alarm.
// The flag blocks all panic logic until the button is released.
static bool waitingForRelease = false;

// ── Handle Panic Button & Alarm Logic ─────────────────────────
void updateAlarmState() {
  unsigned long nowMs = millis();
  bool panicPressed = isPanicButtonPressed();
  bool freshPress = panicPressed && !prevPanicPressed;
  prevPanicPressed = panicPressed;

  // ── Release lockout: swallow all input until finger lifts ──
  // Set after a successful hold-reset; cleared the moment the
  // button is no longer pressed. Prevents the still-held button
  // from immediately re-triggering the alarm on the next tick.
  if (waitingForRelease) {
    if (!panicPressed && !isInDebounce()) waitingForRelease = false;
    return;
  }

  // ── Stage 5 cancel takes priority over normal panic logic ──
  // If we're in the fall-detection countdown, a fresh press
  // cancels the countdown and CONSUMES the press — it does not
  // also trigger the manual panic alarm.
  //
  // After consuming the press, engage the release lockout: the
  // button is almost certainly still physically held down (typical
  // press is 100–300 ms), and without the lockout the very next
  // tick would see panicPressed==true with no prior state and
  // immediately fire the manual panic alarm. The lockout clears
  // the moment the button is released.
  if (freshPress && fallStage5_userCancel()) {
    waitingForRelease = true;
    return;
  }

  if (!alarmActive) {
    if (panicPressed) {
      alarmActive = true;
      lastBuzzerToggleTime = nowMs;
      buzzerOn = true;
      digitalWrite(BUZZER_PIN, HIGH);
      sendPanic(false);   // Notify phone: isFall = false (manual panic)
    }
  } 
  else {
    // Handle panic button hold to reset alarm
    if (panicPressed) {
      if (!trackingHoldTime) {
        buttonPressedStartTime = nowMs;
        trackingHoldTime = true;
      } else if (nowMs - buttonPressedStartTime >= 3000) {
        alarmActive = false;
        buzzerOn = false;
        digitalWrite(BUZZER_PIN, LOW);
        trackingHoldTime = false;
        waitingForRelease = true; // Block until the button is physically released
      }
    } 
    else {
      if (!isInDebounce()) {
        trackingHoldTime = false;
      }
    }

    // Toggle buzzer every second when alarm is active
    if (alarmActive) {
      if (nowMs - lastBuzzerToggleTime >= 1000) {
        buzzerOn = !buzzerOn;
        digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
        lastBuzzerToggleTime = nowMs;
      }
    }
  }
}

// ── Get alarm status ──────────────────────────────────────────
bool isAlarmActive() {
  return alarmActive;
}

// ── Get reset in progress status ──────────────────────────────
bool isResettingAlarm() {
  return trackingHoldTime;
}

// ── Initialize Alarm/Buzzer Hardware ──────────────────────────
void initializeAlarmHardware() {
  setupButtons();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void alarmTask() {
  // Define the three phases of the alarm
  enum AlarmPhase { PHASE_1_PANIC, PHASE_2_SWEEP, PHASE_3_STACCATO };

  if (!isAlarmActive()) {
    tone(BUZZER_PIN, 0); // Ensure buzzer is off
    return; // Don't run the alarm sequence if the alarm isn't active
  }
  
  // Static variables keep their values between function calls
  static AlarmPhase currentPhase = PHASE_1_PANIC;
  static unsigned long previousMillis = 0;
  
  // Phase 1 trackers
  static int p1_count = 0;
  static bool p1_isHighNote = false; // Tracks if we are currently playing 4000Hz
  
  // Phase 2 trackers
  static int p2_freq = 2000;
  
  // Phase 3 trackers
  static int p3_count = 0;
  static bool p3_isSilent = false;
  
  unsigned long currentMillis = millis();

  // Kickstart the very first tone the first time the task runs
  static bool isFirstRun = true;
  if (isFirstRun) {
    tone(BUZZER_PIN, 3500);
    previousMillis = currentMillis;
    isFirstRun = false;
    return;
  }

  // --- The State Machine ---
  switch (currentPhase) {
    
    // ---------------------------------------------------------
    // Phase 1: Rapid High-Pitch Alternation
    // ---------------------------------------------------------
    case PHASE_1_PANIC:
      if (currentMillis - previousMillis >= 70) {
        previousMillis = currentMillis; // Reset the timer
        
        if (p1_isHighNote) {
          p1_count++;
          if (p1_count >= 15) {
            // Reached 15 iterations, transition to Phase 2
            currentPhase = PHASE_2_SWEEP;
            p1_count = 0;
            p1_isHighNote = false;
            p2_freq = 2000;
            tone(BUZZER_PIN, p2_freq); // Kick off Phase 2 tone
            break;
          }
          tone(BUZZER_PIN, 3500); // Back to the lower note
          p1_isHighNote = false;
        } else {
          tone(BUZZER_PIN, 4000); // Up to the higher note
          p1_isHighNote = true;
        }
      }
      break;

    // ---------------------------------------------------------
    // Phase 2: The Fast Sweep
    // ---------------------------------------------------------
    case PHASE_2_SWEEP:
      if (currentMillis - previousMillis >= 5) {
        previousMillis = currentMillis;
        p2_freq += 50;
        
        if (p2_freq >= 5000) {
          // Reached the top of the sweep, transition to Phase 3
          currentPhase = PHASE_3_STACCATO;
          p3_count = 0;
          p3_isSilent = false;
          tone(BUZZER_PIN, 3800); // Kick off Phase 3 tone
          break;
        }
        tone(BUZZER_PIN, p2_freq); // Play the next step in the sweep
      }
      break;

    // ---------------------------------------------------------
    // Phase 3: Short, Aggressive Staccato Beeps
    // ---------------------------------------------------------
    case PHASE_3_STACCATO:
      // The interval changes depending on whether we are currently beeping or silent
      unsigned long currentInterval = p3_isSilent ? 50 : 100;
      
      if (currentMillis - previousMillis >= currentInterval) {
        previousMillis = currentMillis;
        
        if (p3_isSilent) {
          p3_count++;
          if (p3_count >= 4) {
            // Reached 4 beeps, restart the whole sequence at Phase 1
            currentPhase = PHASE_1_PANIC;
            p3_count = 0;
            tone(BUZZER_PIN, 3500); // Kick off Phase 1 tone
            break;
          }
          tone(BUZZER_PIN, 3800); // Play beep
          p3_isSilent = false;
        } else {
          noTone(BUZZER_PIN); // Stop sound for the silence gap
          p3_isSilent = true;
        }
      }
      break;
  }
}