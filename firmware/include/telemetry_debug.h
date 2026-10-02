#ifndef TELEMETRY_DEBUG_H
#define TELEMETRY_DEBUG_H

#include <Arduino.h>
#include "sensor_imu.h"
#include "sensor_vitals.h"
#include "alarm_buzzer.h"
#include "fall_detection.h"

// ── Task: Serial Telemetry Debug Printing (Every 500ms) ───────
void printSerialDebug();

#endif
