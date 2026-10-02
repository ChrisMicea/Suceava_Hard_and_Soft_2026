    #include "sensor_imu.h"
    #include "fall_detection.h"
    #include <queue>

    // ── IMU Global Variables ──────────────────────────────────────
    volatile bool IMU_data_ready = false;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    std::queue<IMUData> accelDataQueue;
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
    void readIMU() {
    if (!IMU_data_ready) return;       
    IMU_data_ready = false; 
    cnt++;

    uint8_t intStatus;
    mpuRead(REG_INT_STATUS, &intStatus, 1);
    if (!(intStatus & 0x01)) return; // Fired due to noise or unrelated register trigger

    uint8_t buf[14];
    mpuRead(REG_ACCEL_XOUT_H, buf, 14);

    ax = (int16_t)((buf[0]  << 8) | buf[1]);
    ay = (int16_t)((buf[2]  << 8) | buf[3]);
    az = (int16_t)((buf[4]  << 8) | buf[5]);
    gx = (int16_t)((buf[8]  << 8) | buf[9]);
    gy = (int16_t)((buf[10] << 8) | buf[11]);
    gz = (int16_t)((buf[12] << 8) | buf[13]);

    IMUData newData = { ax, ay, az, gx, gy, gz };
    accelDataQueue.push(newData);

    if (accelDataQueue.size() > 60) {
        accelDataQueue.pop();
    }
    }

    // ── Initialize MPU-6050 ───────────────────────────────────────
    void initializeMPU() {
    mpuWrite(REG_PWR_MGMT_1, 0x00);
    delay(100);
    mpuWrite(REG_SMPLRT_DIV, 0x09); // Sample frequency rate math balance logic
    mpuWrite(REG_CONFIG, 0x03);     // Turn on low-pass filtering to clear motion noise
    mpuWrite(REG_ACCEL_CONFIG, 0x10);
    mpuWrite(REG_GYRO_CONFIG, 0x08);
    mpuWrite(REG_INT_PIN_CFG, 0x00);
    mpuWrite(REG_INT_ENABLE, 0x01); // Trigger data ready pin state alerts

    // Clear stale buffers from MPU register space before turning on monitoring
    uint8_t dummy;
    mpuRead(REG_INT_STATUS, &dummy, 1);

    // Bind the Hardware interrupt pin directly into RAM runtime space
    pinMode(INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INT_PIN), onMpuInterrupt, RISING);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)INT_PIN, 1); // Allow deep wake operations via high state pins

    Serial.println("MPU-6050 initialized.");
    }