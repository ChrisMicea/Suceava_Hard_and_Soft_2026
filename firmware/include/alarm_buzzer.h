#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include <Arduino.h>
#include "hardware_config.h"

// ── Alarm State Tracking Variables ────────────────────────────
extern bool alarmActive;
extern bool buzzerOn;
extern unsigned long lastBuzzerToggleTime;
extern unsigned long buttonPressedStartTime;
extern bool trackingHoldTime;

// ── Handle Panic Button & Alarm Logic ─────────────────────────
void updateAlarmState();

// ── Get alarm status ──────────────────────────────────────────
bool isAlarmActive();

// ── Get reset in progress status ──────────────────────────────
bool isResettingAlarm();

// ── Initialize Alarm/Buzzer Hardware ──────────────────────────
void initializeAlarmHardware();

void alarmTask();

#endif
