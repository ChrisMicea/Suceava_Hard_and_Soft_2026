#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "hardware_config.h"
#include "alarm_buzzer.h"
#include "sensor_vitals.h"

// ── OLED Display Instance ─────────────────────────────────────
extern Adafruit_SSD1306 display;

// ── Display State Tracking ────────────────────────────────────
extern bool isCurrentlyMirrored;

// ── Handle Screen Rotation/Mirroring ──────────────────────────
void updateScreenMirror();

// ── Draw the alarm strobe screen ──────────────────────────────
void drawAlarmScreen();

// ── Draw the normal vitals display ────────────────────────────
void drawVitalsScreen();

// ── Task: UI, Buttons, Alarm & Screen Painter (Every 50ms) ────
void updateDisplayAndStates();

// ── Initialize OLED Display Hardware ──────────────────────────
void initializeDisplay();

// ── Initialize Display Control Buttons ────────────────────────
void initializeDisplayButtons();

// ── Draw the idle screen ──────────────────
void drawIdleScreen();


#endif
