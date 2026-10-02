#include <Arduino.h>
#include <Wire.h>

// ── Pin / address ─────────────────────────────────────────────
#define MPU_ADDR 0x68
#define INT_PIN 4 // MPU-6050 INT → ESP32 GPIO 4

// ── MPU-6050 registers ────────────────────────────────────────
#define REG_PWR_MGMT_1  0x6B
#define REG_SMPLRT_DIV  0x19
#define REG_CONFIG      0x1A
#define REG_ACCEL_CONFIG 0x1C
#define REG_GYRO_CONFIG 0x1B
#define REG_INT_PIN_CFG 0x37
#define REG_INT_ENABLE  0x38
#define REG_INT_STATUS  0x3A
#define REG_ACCEL_XOUT_H 0x3B

// extra register for motion detection threshold for firing the interrupt
#define REG_MOT_THR          0x1F
#define REG_MOT_DUR          0x20
#define REG_MOT_DETECT_CTRL  0x69
#define REG_MOT_DETECT_STATUS 0x61

// ── Shared flag ───────────────────────────────────────────────
volatile bool dataReady = false;

// temporary global variables to avoid overcomplicating sendData()
int16_t ax, ay, az;
int16_t gx, gy, gz;

uint32_t lastMotionMs = 0;
const uint32_t IDLE_TIMEOUT_MS = 3000;  // sleep after 3s of stillness

// Add a mode flag
enum ImuMode { MODE_MOTION_DETECT, MODE_STREAMING };
volatile ImuMode imuMode = MODE_MOTION_DETECT;


// ── I²C helpers ───────────────────────────────────────────────
void mpuWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void mpuRead(uint8_t reg, uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);          // repeated start
  Wire.requestFrom(MPU_ADDR, (int)len);
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
}

void setMotionWakeMode() {
    // 1. Enable accelerometer low-power cycle mode
    //    CYCLE=1 (bit5), SLEEP=0, TEMP_DIS=1 (bit3) to save more power
    mpuWrite(REG_PWR_MGMT_1, 0x28);

    // 2. Motion threshold: 1 LSB = 2mg
    //    20 → ~40mg — adjust to taste
    //    Higher = less sensitive, won't wake on tiny vibrations
    mpuWrite(REG_MOT_THR, 20);

    // 3. Duration: motion must persist for N milliseconds
    //    5ms avoids false triggers from single spikes
    mpuWrite(REG_MOT_DUR, 5);

    // 4. Accel settling time after wake — datasheet says set bits [5:4]
    mpuWrite(REG_MOT_DETECT_CTRL, 0x15);  // ACCEL_ON_DELAY=1, MOT_COUNT=5

    // 5. Enable ONLY the motion interrupt (bit 6), not DATA_RDY
    mpuWrite(REG_INT_ENABLE, 0x40);
}

void setDataReadyMode() {
    // Switch back to full-power normal operation
    mpuWrite(REG_PWR_MGMT_1,   0x00);
    delay(50);  // let accel stabilise

    // Restore sample rate and DLPF
    mpuWrite(REG_SMPLRT_DIV,   0x09);
    mpuWrite(REG_CONFIG,       0x03);

    // Re-enable DATA_RDY interrupt only
    mpuWrite(REG_INT_ENABLE,   0x01);
}

void checkIdleTimeout() {
    if (imuMode == MODE_STREAMING && 
        millis() - lastMotionMs > IDLE_TIMEOUT_MS) {
        Serial.println("Idle timeout, returning to motion detect.");
        imuMode = MODE_MOTION_DETECT;
        setMotionWakeMode();
    }
}

void IRAM_ATTR onMpuInterrupt() {
  dataReady = true;
}

// Each sensor gets a simple task descriptor
struct Task {
    uint32_t intervalMs;
    uint32_t lastRunMs;
    void (*handler)();
};

void readSpO2() {
  ;
}

void readIMU() {
  if (!dataReady) // nothing to do
    return;       

  dataReady = false; // clear before reading, not after

  uint8_t intStatus;
  mpuRead(REG_INT_STATUS, &intStatus, 1);

  if (imuMode == MODE_MOTION_DETECT) {
    if (!(intStatus & 0x40)) return;  // bit 6 = MOT

    // Motion detected — switch to streaming mode
    Serial.println("Motion detected, starting stream.");
    imuMode = MODE_STREAMING;
    setDataReadyMode();

    // Reset idle timer
    lastMotionMs = millis();
  }
  else {
    if (!(intStatus & 0x01)) // DATA_RDY bit (LSB if intStatus) not set — INT pin fired for unclear reson / electrical noise
      return;


    // Read 14 bytes: accel(6) + temp(2) + gyro(6)
    uint8_t buf[14];
    mpuRead(REG_ACCEL_XOUT_H, buf, 14);

    ax = (int16_t)((buf[0]  << 8) | buf[1]);
    ay = (int16_t)((buf[2]  << 8) | buf[3]);
    az = (int16_t)((buf[4]  << 8) | buf[5]);
    // buf[6..7] = temperature (skipped)
    gx = (int16_t)((buf[8]  << 8) | buf[9]);
    gy = (int16_t)((buf[10] << 8) | buf[11]);
    gz = (int16_t)((buf[12] << 8) | buf[13]);

    lastMotionMs = millis();  // reset idle timer on every sample
  }
}

void readHR() {
  ;
}

void sendData() {
  if (imuMode != MODE_STREAMING) return;  // don't send stale data

  // ±2 g → 16384 LSB/g
  // ±250°/s → 131 LSB/(°/s)
  Serial.printf("%.3f\t%.3f\t%.3f\t%.2f\t%.2f\t%.2f\n",
    ax / 16384.0f, ay / 16384.0f, az / 16384.0f,
    gx / 131.0f,   gy / 131.0f,   gz / 131.0f);
}

Task tasks[] = {
    { 10, 0, readIMU },  // 100 Hz — but really driven by ISR flag
    { 5000, 0, readHR },  // every 5s
    { 5000, 0, readSpO2 },  // every 5s (offset from HR if needed)
    { 200, 0, sendData },  // every 1s transmit
    { 1000, 0, checkIdleTimeout}, // every 1 second
};

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  delay(100);

  // 1. Wake from sleep
  mpuWrite(REG_PWR_MGMT_1, 0x00);
  delay(100);

  // 2. 100 Hz sample rate: gyro runs at 1 kHz with DLPF on → 1000/(1+9)=100
  mpuWrite(REG_SMPLRT_DIV, 0x09);

  // 3. DLPF ~44 Hz — required for 1 kHz gyro clock (DLPF=0 uses 8 kHz clock,
  //    making SMPLRT_DIV behave differently and the INT fire way too fast)
  mpuWrite(REG_CONFIG, 0x03);

  // 4. ±2 g, ±250 °/s
  mpuWrite(REG_ACCEL_CONFIG, 0x00);
  mpuWrite(REG_GYRO_CONFIG, 0x00);

  // 5. INT pin: active-high, push-pull, clears on any read (bit5=0 latch off)
  //    0x00 is the reset default and works fine for RISING edge detection.
  mpuWrite(REG_INT_PIN_CFG, 0x00);

  // 6. Enable Data Ready interrupt
  mpuWrite(REG_INT_ENABLE, 0x01);

  // 7. Clear any stale interrupt BEFORE attaching the ISR
  uint8_t dummy;
  mpuRead(REG_INT_STATUS, &dummy, 1);

  // 8. Configure GPIO and attach ISR  ← THIS WAS MISSING
  pinMode(INT_PIN, INPUT);              // MPU is push-pull, no pull needed
  attachInterrupt(digitalPinToInterrupt(INT_PIN), onMpuInterrupt, RISING);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)INT_PIN, 1); // wake on HIGH

  setMotionWakeMode();  // start quiet, wake on movement
  imuMode = MODE_MOTION_DETECT;
  lastMotionMs = millis();  // don't idle-timeout immediately on boot

  // ── Verify registers ──────────────────────────────────────
  uint8_t val;
  mpuRead(REG_PWR_MGMT_1,   &val, 1); Serial.printf("PWR_MGMT_1   (0x6B): 0x%02X\n", val);
  mpuRead(REG_SMPLRT_DIV,   &val, 1); Serial.printf("SMPLRT_DIV   (0x19): 0x%02X\n", val);
  mpuRead(REG_CONFIG,       &val, 1); Serial.printf("CONFIG       (0x1A): 0x%02X\n", val);
  mpuRead(REG_ACCEL_CONFIG, &val, 1); Serial.printf("ACCEL_CONFIG (0x1C): 0x%02X\n", val);
  mpuRead(REG_GYRO_CONFIG,  &val, 1); Serial.printf("GYRO_CONFIG  (0x1B): 0x%02X\n", val);
  mpuRead(REG_INT_PIN_CFG,  &val, 1); Serial.printf("INT_PIN_CFG  (0x37): 0x%02X\n", val);
  mpuRead(REG_INT_ENABLE,   &val, 1); Serial.printf("INT_ENABLE   (0x38): 0x%02X\n", val);

  Serial.println("MPU-6050 ready at 100 Hz.");
  Serial.println("ax(g)\tay(g)\taz(g)\tgx(°/s)\tgy(°/s)\tgz(°/s)");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {   
  uint32_t now = millis();
  uint32_t nextWakeIn = UINT32_MAX;

  for (auto& t : tasks) {
      uint32_t elapsed = now - t.lastRunMs;
      if (elapsed >= t.intervalMs) {
          t.handler();
          t.lastRunMs = now;
      } 
      else {
          // how long until this task needs to run?
          nextWakeIn = min(nextWakeIn, t.intervalMs - elapsed);
      }
  }

  // If no task is imminent and no IMU interrupt pending → sleep
  if (!dataReady && nextWakeIn > 5 && nextWakeIn != UINT32_MAX) { // last check in case all tasks run at once
      esp_sleep_enable_timer_wakeup(nextWakeIn * 1000); // µs, sleep until next task
      esp_light_sleep_start();
      // execution resumes HERE after wake
  }
}