#include <Arduino.h>
const int BUZZER_PIN = D5; // Connected to Pin D5 on your ESP8266

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // Phase 1: Rapid High-Pitch Alternation (The "Panic" Sound)
  // Flips between 3500Hz and 4000Hz every 70 milliseconds

  if ()

  for (int i = 0; i < 15; i++) {
    tone(BUZZER_PIN, 3500); // Piercing high note
    delay(70);
    tone(BUZZER_PIN, 4000); // Even higher note
    delay(70);
    
    yield(); // Keep the ESP8266 background processes happy
  }

  // Phase 2: The Fast Sweep (The "Screamer")
  // Sweeps up rapidly from 2000Hz to 5000Hz
  for (int freq = 2000; freq < 5000; freq += 50) {
    tone(BUZZER_PIN, freq);
    delay(5);
    
    yield();
  }

  // Phase 3: Short, Aggressive Staccato Beeps
  // Loud, brief bursts of sound with tiny silences
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, 3800);
    delay(100);
    noTone(BUZZER_PIN); // Total silence segment makes the next beep more jarring
    delay(50);
    
    yield();
  }
}