#ifndef SENSOR_IMU_H
#define SENSOR_IMU_H

#include <Arduino.h>
#include <Wire.h>
#include "hardware_config.h"
#include <queue>

struct IMUData {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
};

// ── IMU Global Variables ──────────────────────────────────────
extern volatile bool IMU_data_ready;
extern int16_t ax, ay, az;
extern int16_t gx, gy, gz;

// ── Low Level I2C Communications Helper Functions ─────────────
void mpuWrite(uint8_t reg, uint8_t value);
void mpuRead(uint8_t reg, uint8_t* buf, uint8_t len);

// ── Hardware ISR ──────────────────────────────────────────────
void IRAM_ATTR onMpuInterrupt();

// ── Task: High Frequency IMU Handler (100 Hz Engine) ──────────
void readIMU();

// ── Initialize MPU-6050 ───────────────────────────────────────
void initializeMPU();

#endif
