#include "telemetry_debug.h"

extern int cnt; // For debug counting of IMU reads

// ── Task: Serial Telemetry Debug Printing (Every 500ms) ───────
void printSerialDebug() {
  // 1. Explicitly cast IMU calculations to float to guarantee formatting match
  Serial.printf("IMU -> AX:%.2f AY:%.2f AZ:%.2f | GX:%.1f GY:%.1f GZ:%.1f\n",
    (float)(ax / 16384.0f), (float)(ay / 16384.0f), (float)(az / 16384.0f),
    (float)(gx / 131.0f),   (float)(gy / 131.0f),   (float)(gz / 131.0f));

  Serial.printf("IMU Read Count: %d\n", cnt);
  cnt = 0; // Reset count after printing to track reads per interval

  // 2. Safe variable copy to decouple from dynamic sensor changes
  float currentHR = body.heartRate;
  uint32_t currentSpO2 = (uint32_t)body.oxygen; // Force cast to a stable integer type
  const char* alarmString = alarmActive ? "YES" : "NO";

  // 3. Print Vitals using completely predictable type definitions
  Serial.printf("Vitals -> HR: %.1f BPM | SpO2: %u%% | Temp: %.2f C | Alarm Active: %s\n\n",
    currentHR, 
    currentSpO2, 
    globalBodyTemp, 
    alarmString);

  Serial.printf("Bio Hub Status -> Confidence: %u%% | Status Code: %u\n\n", 
    body.confidence, body.status);

  // 4. Is on body? (Gate 0 logic)
  Serial.printf("Gate 0 -> Armed: %s | On Body: %s | Time until Armed: %ums\n",
    gate0_isArmed() ? "YES" : "NO",
    gate0_isOnBody() ? "YES" : "NO",
    gate0_msUntilArmed());

  // 5. Is falling? (Fall detection logic)
  //if falling exit program and print "FALLING: YES" else print "FALLING: NO"
//   if (isFalling()) {
//     Serial.println("Fall Detection -> Falling: YES");
//   } else {
//     Serial.println("Fall Detection -> Falling: NO");
//   }

}
