#ifndef SENSOR_IMU_H
#define SENSOR_IMU_H

#include <Arduino.h>
#include <Wire.h>
#include "hardware_config.h"
#include "imu_ring_buffer.h"   // Replaces <queue>

// ── IMU Sample Structure ──────────────────────────────────────
struct IMUData {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
};

// ── Sliding-window sample store ───────────────────────────────
// Declared here so both sensor_imu.cpp and fall_detection.cpp
// share the same instance via extern.
// Capacity 60 = 600 ms at 100 Hz — covers the full fall arc.
extern IMURingBuffer<IMUData, 60> accelDataQueue;

// ── IMU Global Variables ──────────────────────────────────────
extern volatile bool IMU_data_ready;
extern int16_t ax, ay, az;
extern int16_t gx, gy, gz;

// ── Low Level I2C Communications Helper Functions ─────────────
void mpuWrite(uint8_t reg, uint8_t value);
void mpuRead(uint8_t reg, uint8_t* buf, uint8_t len);

// ── Hardware ISR ──────────────────────────────────────────────
void IRAM_ATTR onMpuInterrupt();

// ── Task: High Frequency IMU Handler ──────────────────────────
void readIMU();

// ── Initialize MPU-6050 ───────────────────────────────────────
void initializeMPU();

#endif // SENSOR_IMU_H