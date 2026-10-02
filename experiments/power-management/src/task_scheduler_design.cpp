// #include <Arduino.h>
// // #include <Wire.h>

// // Each sensor gets a simple task descriptor
// struct Task {
//     uint32_t intervalMs;
//     uint32_t lastRunMs;
//     void (*handler)();
// };

// void readSpO2() {
//   ;
// }

// void readIMU() {
//   ;
// }

// void readHR() {
//   ;
// }

// void sendData() {
//   ;
// }

// Task tasks[] = {
//     { 10, 0, readIMU },  // 100 Hz — but really driven by ISR flag
//     { 5000, 0, readHR },  // every 5s
//     { 5000, 0, readSpO2 },  // every 5s (offset from HR if needed)
//     { 1000, 0, sendData },  // every 1s transmit
// };

// void setup() {
//   ;
// }

// void loop() {
//     uint32_t now = millis();
//     uint32_t nextWakeIn = UINT32_MAX;

//     for (auto& t : tasks) {
//         uint32_t elapsed = now - t.lastRunMs;
//         if (elapsed >= t.intervalMs) {
//             t.handler();
//             t.lastRunMs = now;
//         } 
//         else {
//             // how long until this task needs to run?
//             nextWakeIn = min(nextWakeIn, t.intervalMs - elapsed);
//         }
//     }

//     // If no task is imminent and no IMU interrupt pending → sleep
//     if (!dataReady && nextWakeIn > 5) {
//         esp_sleep_enable_timer_wakeup(nextWakeIn * 1000); // µs
//         esp_light_sleep_start();
//         // execution resumes HERE after wake
//     }
// }

