#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "sensor_imu.h"
#include "sensor_vitals.h"
#include "alarm_buzzer.h"
#include "fall_detection.h"


// ── Service UUID ──────────────────────────────────────────────────────────────
#define SERVICE_UUID          "45319765-1234-4521-1234-123456789000"

// ── Characteristic UUIDs ──────────────────────────────────────────────────────
// → Phone (notify)
#define CHAR_VITALS_UUID  "45319765-1234-4521-1234-123456789001"
#define CHAR_MOTION_UUID   "45319765-1234-4521-1234-123456789002"
#define CHAR_TEMP_UUID     "45319765-1234-4521-1234-123456789003"

// ← Phone (write)
#define CHAR_PANIC_AKNOWLEDGE_UUID "45319765-1234-4521-1234-123456789004" // uint8: 1=ACK_PANIC

// ─ Panic (notify → phone)
#define CHAR_PANIC_UUID       "45319765-1234-4521-1234-123456789005" // uint8: 1=PANIC_BUTTON 2=FALL

void setupBLE();
void bluetoothTask();
void sendPanic(bool isFall);

bool isBLEConnected();