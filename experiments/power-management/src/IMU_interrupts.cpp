// #include <Arduino.h>
// #include <Wire.h>

// // ── Pin / address ─────────────────────────────────────────────
// #define MPU_ADDR        0x68
// #define INT_PIN         4       // MPU-6050 INT → ESP32 GPIO 4

// // ── MPU-6050 registers ────────────────────────────────────────
// #define REG_PWR_MGMT_1  0x6B
// #define REG_SMPLRT_DIV  0x19
// #define REG_CONFIG      0x1A
// #define REG_ACCEL_CONFIG 0x1C
// #define REG_GYRO_CONFIG 0x1B
// #define REG_INT_PIN_CFG 0x37
// #define REG_INT_ENABLE  0x38
// #define REG_INT_STATUS  0x3A
// #define REG_ACCEL_XOUT_H 0x3B

// // ── Shared flag ───────────────────────────────────────────────
// volatile bool dataReady = false;

// void IRAM_ATTR onMpuInterrupt() {
//   dataReady = true;
// }

// // ── I²C helpers ───────────────────────────────────────────────
// void mpuWrite(uint8_t reg, uint8_t value) {
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.write(value);
//   Wire.endTransmission();
// }

// void mpuRead(uint8_t reg, uint8_t* buf, uint8_t len) {
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);          // repeated start
//   Wire.requestFrom(MPU_ADDR, (int)len);
//   for (uint8_t i = 0; i < len; i++) {
//     buf[i] = Wire.read();
//   }
// }

// // ── Setup ─────────────────────────────────────────────────────
// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   Wire.begin();
//   delay(100);

//   // 1. Wake from sleep
//   mpuWrite(REG_PWR_MGMT_1, 0x00);
//   delay(100);

//   // 2. 100 Hz sample rate: gyro runs at 1 kHz with DLPF on → 1000/(1+9)=100
//   mpuWrite(REG_SMPLRT_DIV, 0x09);

//   // 3. DLPF ~44 Hz — required for 1 kHz gyro clock (DLPF=0 uses 8 kHz clock,
//   //    making SMPLRT_DIV behave differently and the INT fire way too fast)
//   mpuWrite(REG_CONFIG, 0x03);

//   // 4. ±2 g, ±250 °/s
//   mpuWrite(REG_ACCEL_CONFIG, 0x00);
//   mpuWrite(REG_GYRO_CONFIG, 0x00);

//   // 5. INT pin: active-high, push-pull, clears on any read (bit5=0 latch off)
//   //    0x00 is the reset default and works fine for RISING edge detection.
//   mpuWrite(REG_INT_PIN_CFG, 0x00);

//   // 6. Enable Data Ready interrupt
//   mpuWrite(REG_INT_ENABLE, 0x01);

//   // 7. Clear any stale interrupt BEFORE attaching the ISR
//   uint8_t dummy;
//   mpuRead(REG_INT_STATUS, &dummy, 1);

//   // 8. Configure GPIO and attach ISR  ← THIS WAS MISSING
//   pinMode(INT_PIN, INPUT);              // MPU is push-pull, no pull needed
//   attachInterrupt(digitalPinToInterrupt(INT_PIN), onMpuInterrupt, RISING);

//   // ── Verify registers ──────────────────────────────────────
//   uint8_t val;
//   mpuRead(REG_PWR_MGMT_1,   &val, 1); Serial.printf("PWR_MGMT_1   (0x6B): 0x%02X\n", val);
//   mpuRead(REG_SMPLRT_DIV,   &val, 1); Serial.printf("SMPLRT_DIV   (0x19): 0x%02X\n", val);
//   mpuRead(REG_CONFIG,       &val, 1); Serial.printf("CONFIG       (0x1A): 0x%02X\n", val);
//   mpuRead(REG_ACCEL_CONFIG, &val, 1); Serial.printf("ACCEL_CONFIG (0x1C): 0x%02X\n", val);
//   mpuRead(REG_GYRO_CONFIG,  &val, 1); Serial.printf("GYRO_CONFIG  (0x1B): 0x%02X\n", val);
//   mpuRead(REG_INT_PIN_CFG,  &val, 1); Serial.printf("INT_PIN_CFG  (0x37): 0x%02X\n", val);
//   mpuRead(REG_INT_ENABLE,   &val, 1); Serial.printf("INT_ENABLE   (0x38): 0x%02X\n", val);

//   Serial.println("MPU-6050 ready at 100 Hz.");
//   Serial.println("ax(g)\tay(g)\taz(g)\tgx(°/s)\tgy(°/s)\tgz(°/s)");
// }

// // ── Loop ──────────────────────────────────────────────────────
// void loop() {
//   if (!dataReady) // yield immediately — no blocking, no lost data 
//     return;   

//   // Clear flag BEFORE reading so a new interrupt that fires during
//   // the I²C transaction is not silently dropped.
//   dataReady = false;

//   // Clear INT_STATUS so the MPU de-asserts the pin (auto-clears on read
//   // when latch mode is off, which is our config, but explicit is safer)
//   uint8_t intStatus;
//   mpuRead(REG_INT_STATUS, &intStatus, 1);
//   if (!(intStatus & 0x01)) // DATA_RDY bit (LSB if intStatus) not set — INT pin fired for unclear reson / electrical noise
//     return;  

//   // Read 14 bytes: accel(6) + temp(2) + gyro(6)
//   uint8_t buf[14];
//   mpuRead(REG_ACCEL_XOUT_H, buf, 14);

//   int16_t ax = (int16_t)((buf[0]  << 8) | buf[1]);
//   int16_t ay = (int16_t)((buf[2]  << 8) | buf[3]);
//   int16_t az = (int16_t)((buf[4]  << 8) | buf[5]);
//   // buf[6..7] = temperature (skipped)
//   int16_t gx = (int16_t)((buf[8]  << 8) | buf[9]);
//   int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
//   int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

//   // ±2 g  → 16384 LSB/g
//   // ±250°/s → 131 LSB/(°/s)
//   Serial.printf("%.3f\t%.3f\t%.3f\t%.2f\t%.2f\t%.2f\n",
//     ax / 16384.0f, ay / 16384.0f, az / 16384.0f,
//     gx / 131.0f,   gy / 131.0f,   gz / 131.0f);
// }