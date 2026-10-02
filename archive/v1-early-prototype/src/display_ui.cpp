#include "display_ui.h"

// ── OLED Display Instance ─────────────────────────────────────
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Display State Tracking ────────────────────────────────────
bool isCurrentlyMirrored = false;

// ── Handle Screen Rotation/Mirroring ──────────────────────────
void updateScreenMirror() {
  bool mirrorPressed = (digitalRead(MIRROR_BUTTON_PIN) == LOW);
  
  if (mirrorPressed && !isCurrentlyMirrored) {
    display.ssd1306_command(0xA0); 
    display.ssd1306_command(0xC8); 
    isCurrentlyMirrored = true;
  } 
  else if (!mirrorPressed && isCurrentlyMirrored) {
    display.ssd1306_command(0xA1); 
    display.ssd1306_command(0xC8); 
    isCurrentlyMirrored = false;
  }
}

// ── Draw the alarm strobe screen ──────────────────────────────
void drawAlarmScreen() {
  if (isResettingAlarm()) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.println("System Monitor");
    display.drawFastHLine(0, 10, 128, WHITE);

    display.setCursor(0, 28);
    display.print(" RESETTING ALARM... ");
  } 
  else {
    display.fillRect(0, 0, 128, 64, WHITE); // Alarm strobe effect
  }
}

// ── Draw the normal vitals display ────────────────────────────
void drawVitalsScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("Elderly Band Local");
  display.drawFastHLine(0, 10, 128, WHITE);

  // Heart Rate Allocation
  display.setCursor(0, 16);
  display.print("Heart Rate: ");
  if (bioHubReady) { 
    display.print(body.heartRate); 
    display.print(" BPM"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }

  // Blood Oxygen Allocation
  display.setCursor(0, 32);
  display.print("Blood SpO2: ");
  if (bioHubReady) { 
    display.print(body.oxygen); 
    display.print(" %"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }

  // Temperature Allocation
  display.setCursor(0, 48);
  display.print("Body Temp:  ");
  if (tempSensorReady) { 
    display.print(globalBodyTemp, 1); 
    display.print(" C"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }
}

// ── Task: UI, Buttons, Alarm & Screen Painter (Every 50ms) ────
void updateDisplayAndStates() {
  // Handle screen rotation
  updateScreenMirror();
  
  // Handle alarm state and button logic
  updateAlarmState();

  // Paint the display
  display.clearDisplay();

  if (isAlarmActive()) {
    drawAlarmScreen();
  } 
  else {
    drawVitalsScreen();
  }

  display.display();
}

// ── Initialize OLED Display Hardware ──────────────────────────
void initializeDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { 
    Serial.println(F("OLED allocation failed. Running raw headless mode."));
  }
  display.clearDisplay();
  display.display();
}

// ── Initialize Display Control Buttons ────────────────────────
void initializeDisplayButtons() {
  pinMode(MIRROR_BUTTON_PIN, INPUT_PULLUP);
}
