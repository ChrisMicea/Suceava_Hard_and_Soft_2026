#include "sensor_imu.h"
#include "fall_detection.h"

// ── IMU Global Variables ──────────────────────────────────────
volatile bool IMU_data_ready = false;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Ring buffer: 60 samples × 12 bytes = 720 bytes in BSS.
// Replaces std::queue<IMUData> which used heap-allocated std::deque
// chunks — fragmentation-prone on a 300 KB MCU with no compactor.
// Capacity 60 = 600 ms of history at 100 Hz; enough for the full
// free-fall → impact arc (~300–500 ms) plus margin.
IMURingBuffer<IMUData, 60> accelDataQueue;

extern int cnt; // For debug counting of IMU reads

// ── Low Level I2C Communications Helper Functions ─────────────
void mpuWrite(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void mpuRead(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false); // Repeated start
    Wire.requestFrom(MPU_ADDR, (int)len);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
}

// ── Hardware ISR ──────────────────────────────────────────────
void IRAM_ATTR onMpuInterrupt() {
    IMU_data_ready = true;
}

// ── Task: High Frequency IMU Handler (100 Hz Engine) ──────────
// Called from the ISR-flagged drainer in loop() — NOT from inside
// the ISR itself, so I2C calls are safe here.
//
// push() uses a portMUX spinlock internally, so concurrent access
// from the scheduler tasks (isFalling, isCurrentlyRestful) is safe
// without additional locking at this call site.
void readIMU() {
    if (!IMU_data_ready) return;
    IMU_data_ready = false;
    cnt++;

    uint8_t intStatus;
    mpuRead(REG_INT_STATUS, &intStatus, 1);
    if (!(intStatus & 0x01)) return; // Spurious interrupt — ignore

    uint8_t buf[14];
    mpuRead(REG_ACCEL_XOUT_H, buf, 14);

    ax = (int16_t)((buf[0]  << 8) | buf[1]);
    ay = (int16_t)((buf[2]  << 8) | buf[3]);
    az = (int16_t)((buf[4]  << 8) | buf[5]);
    gx = (int16_t)((buf[8]  << 8) | buf[9]);
    gy = (int16_t)((buf[10] << 8) | buf[11]);
    gz = (int16_t)((buf[12] << 8) | buf[13]);

    // push() silently evicts the oldest sample when the buffer is full,
    // preserving the most recent 60-sample sliding window without any
    // explicit size-check + pop() dance.
    accelDataQueue.push({ ax, ay, az, gx, gy, gz });
}

// ── Initialize MPU-6050 ───────────────────────────────────────
void initializeMPU() {
    mpuWrite(REG_PWR_MGMT_1, 0x00);
    delay(100);
    mpuWrite(REG_SMPLRT_DIV, 0x09); // 100 Hz output data rate (1 kHz / (9+1))
    mpuWrite(REG_CONFIG, 0x03);     // DLPF ~44 Hz — cuts vibration noise
    mpuWrite(REG_ACCEL_CONFIG, 0x10); // ±8 g full scale (4096 LSB/g)
    mpuWrite(REG_GYRO_CONFIG, 0x18);  // ±2000 °/s full scale (16.4 LSB/(°/s))
                                      // Crutch-tip data shows 250+ °/s peaks during
                                      // normal walking and 400+ during falls;
                                      // ±500 °/s saturated. ±2000 leaves headroom.
    mpuWrite(REG_INT_PIN_CFG, 0x00);
    mpuWrite(REG_INT_ENABLE, 0x01);   // Data-ready interrupt enable

    // Drain the INT_STATUS register so the first ISR fires on fresh data.
    uint8_t dummy;
    mpuRead(REG_INT_STATUS, &dummy, 1);

    // Attach the hardware interrupt and enable it as a light-sleep wakeup
    // source so the ESP32 can sleep between IMU samples.
    pinMode(INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INT_PIN), onMpuInterrupt, RISING);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)INT_PIN, 1);

    Serial.println("MPU-6050 initialized.");
}