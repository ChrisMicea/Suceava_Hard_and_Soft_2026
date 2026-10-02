#include <Arduino.h>
#include <Wire.h>

// ── Library Includes ──────────────────────────────────────────
#include "hardware_config.h"
#include "sensor_imu.h"
#include "sensor_vitals.h"
#include "alarm_buzzer.h"
#include "display_ui.h"
#include "task_scheduler.h"
#include "telemetry_debug.h"
#include "fall_detection.h"
#include "BLE_connection.h"
#include "buttons.h"
#include <vector>

// ── Cooperative Scheduler Task Array ──────────────────────────
Task tasks[] = {
    { 10,   0, readIMU },                // 100 Hz - IMU data via interrupt
    { 50,   0, updateDisplayAndStates }, // 20 Hz - UI and display updates
    { 500,  0, printSerialDebug },       // Debug telemetry every 500ms
    { 500,  0, evaluateFall },           // Check fall detection logic every 500ms
    { 200,  0, readBioHub },             // Pulse oximeter every 200ms
    { 3000, 0, readTemperature },         // Temperature sensor every 3s
    { 200,  0, updateGate0 },             // Gate 0 logic every 200ms to evaluate on-body status
    { 100,  0, bluetoothTask },
    { 100, 0, alarmTask}            // BLE communication task every 200ms
};

const uint8_t TASK_COUNT = sizeof(tasks) / sizeof(tasks[0]);
int cnt = 0;


// ══════════════════════════════════════════════════════════════
// SETUP: Initialize all hardware and systems
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // 100kHz for legacy I2C chips

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // Initialize hardware subsystems
  initializeAlarmHardware();

  initializeDisplay();
  initializeBioHub();
  initializeTemperatureSensor();
  initializeMPU();
  setupBLE();

  Serial.println("✓ All systems online and ready.");
}

// ══════════════════════════════════════════════════════════════
// LOOP: Cooperative task scheduler
// ══════════════════════════════════════════════════════════════
void loop() {   

  if (gate0_isArmed() || gate0_isOnBody()) {
    digitalWrite(LED1_PIN, HIGH);
  } else {
    digitalWrite(LED1_PIN, LOW);
  }

  if (isBLEConnected()) {
    digitalWrite(LED2_PIN, HIGH);
  } else {
    digitalWrite(LED2_PIN, LOW);
  }

  while (IMU_data_ready) // drain every interrupt, no timer gating
    readIMU();  
  uint32_t nextWakeIn = runTaskScheduler(tasks, TASK_COUNT);

  // Power Optimization: Sleep if no immediate tasks
  if (!IMU_data_ready && nextWakeIn > 5 && nextWakeIn != UINT32_MAX) {
    // esp_sleep_enable_timer_wakeup(nextWakeIn * 1000);
    // esp_light_sleep_start();
  }
}



// #include <SparkFun_Bio_Sensor_Hub_Library.h>
// #include <Wire.h>

// // Reset pin, MFIO pin
// int resPin = 4;
// int mfioPin = 5;

// // MAXIMIZED PULSE WIDTH: Gives the LED more time to shine, increasing penetration depth
// // Options: 69, 118, 215, 411us
// int width = 411; 

// // OPTIMIZED SAMPLE RATE: Lowering this allows the hardware to handle the 411us pulse width stably
// // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200 samples/second
// int samples = 100; 

// int pulseWidthVal;
// int sampleVal;

// SparkFun_Bio_Sensor_Hub bioHub(resPin, mfioPin); 
// bioData body; 

// void setup(){
//   Serial.begin(115200);
//   Wire.begin();
  
//   int result = bioHub.begin();
//   if (result == 0) {
//     Serial.println("Sensor started!");
//   } else {
//     Serial.println("Sensor failed to start. Check wiring!");
//   }

//   Serial.println("Configuring Sensor...."); 
  
//   // USING MODE_ONE: Focuses purely on BPM and SpO2 calculations
//   int error = bioHub.configSensorBpm(MODE_ONE); 
//   if (error == 0){
//     Serial.println("Sensor configured.");
//   } else {
//     Serial.print("Error configuring sensor: "); 
//     Serial.println(error); 
//   }

//   // Set pulse width
//   error = bioHub.setPulseWidth(width);
//   if (error == 0){
//     Serial.println("Pulse Width Set.");
//   } else {
//     Serial.print("Could not set Pulse Width. Error: "); 
//     Serial.println(error); 
//   }

//   // Check pulse width
//   pulseWidthVal = bioHub.readPulseWidth();
//   Serial.print("Confirmed Pulse Width: ");
//   Serial.println(pulseWidthVal);

//   // Set sample rate
//   error = bioHub.setSampleRate(samples);
//   if (error == 0){
//     Serial.println("Sample Rate Set.");
//   } else {
//     Serial.print("Could not set Sample Rate! Error: "); 
//     Serial.println(error); 
//   }

//   // Check sample rate
//   sampleVal = bioHub.readSampleRate();
//   Serial.print("Confirmed Sample Rate: ");
//   Serial.println(sampleVal); 
  
//   // Give the sensor extra time to calibrate against the air gap ambient baseline
//   Serial.println("Loading up the buffer with data (keep arm still)....");
//   delay(5000); 
// }

// void loop(){
//     // Read the processed biometrics from the Sensor Hub's internal M4 microcontroller
//     body = bioHub.readSensorBpm();
    
//     Serial.println("\n--- Sensor Readout ---");
//     Serial.print("Infrared LED counts: "); Serial.println(body.irLed); 
//     Serial.print("Red LED counts:      "); Serial.println(body.redLed); 
//     Serial.print("Heartrate:           "); Serial.println(body.heartRate); 
//     Serial.print("Blood Oxygen (SpO2): "); Serial.println(body.oxygen); 
//     Serial.print("Signal Confidence:   "); Serial.print(body.confidence); Serial.println("%");
    
//     // Status breakdown: 0 = No object, 1 = Object detected, 2 = Finger/Skin detected
//     Serial.print("Status Code:         "); Serial.println(body.status); 

//     // Changed delay to 400ms: Match the 100Hz sampling rate so the serial monitor 
//     // updates fluidly without dropping human heartbeats out of the visual frame.
//     delay(80); 
// }