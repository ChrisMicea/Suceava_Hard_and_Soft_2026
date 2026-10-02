#include "alarm_buzzer.h"

// ── Alarm State Tracking Variables ────────────────────────────
bool alarmActive = false;
bool buzzerOn = false;
unsigned long lastBuzzerToggleTime = 0;
unsigned long buttonPressedStartTime = 0;
bool trackingHoldTime = false;

// ── Handle Panic Button & Alarm Logic ─────────────────────────
void updateAlarmState() {
  unsigned long nowMs = millis();
  bool panicPressed = (digitalRead(PANIC_BUTTON_PIN) == LOW);

  if (!alarmActive) {
    if (panicPressed) {
      alarmActive = true;
      lastBuzzerToggleTime = nowMs;
      buzzerOn = true;
      digitalWrite(BUZZER_PIN, HIGH);
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
      }
    } 
    else {
      trackingHoldTime = false;
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
  pinMode(PANIC_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}
