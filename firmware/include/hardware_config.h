#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// ── Pin / Hardware Assignments (ESP32 Mapping) ────────────────
#define INT_PIN           4   // MPU-6050 INT Pin
#define RES_PIN           13  // Bio Sensor Hub Reset Pin
#define MFIO_PIN          14  // Bio Sensor Hub MFIO Pin
#define BUTTON_PIN        33  // Panic Button Input (with pull-up)
#define TOUCH_PIN         12   // Touch Input 
#define BUZZER_PIN        27  // Audible Warning Buzzer
#define I2C_SDA           21  // ESP32 Standard SDA
#define I2C_SCL           22  // ESP32 Standard SCL
#define LED1_PIN          16  // Status LED 1 (e.g. Power)
#define LED2_PIN          17  // Status LED 2 (e.g. Bluetooth)

// ── MPU-6050 Registers ────────────────────────────────────────
#define MPU_ADDR         0x68
#define REG_PWR_MGMT_1   0x6B
#define REG_SMPLRT_DIV   0x19
#define REG_CONFIG       0x1A
#define REG_ACCEL_CONFIG 0x1C
#define REG_GYRO_CONFIG  0x1B
#define REG_INT_PIN_CFG  0x37
#define REG_INT_ENABLE   0x38
#define REG_INT_STATUS   0x3A
#define REG_ACCEL_XOUT_H 0x3B

// ── OLED Display Configuration ────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3D

#endif
