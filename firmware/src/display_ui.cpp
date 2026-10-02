#include "display_ui.h"
#include "buttons.h"
#include "fall_detection.h"

// ── OLED Display Instance ─────────────────────────────────────
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Display State Tracking ────────────────────────────────────
bool isCurrentlyMirrored = false;

int lastPulse, lastSpO2; // for displaying only non-zero values
float lastTemp; 

// ── Handle Screen Rotation/Mirroring ──────────────────────────
void updateScreenMirror() {
  bool mirrorPressed = isOtherButtonPressed();
  
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
  display.println("Elderly Crutch Local");
  display.drawFastHLine(0, 10, 128, WHITE);

  // Heart Rate Allocation
  display.setCursor(0, 12);
  display.print("Heart Rate: ");
  if (bioHubReady) { 
    if (body.heartRate != 0) {
      lastPulse = body.heartRate;
    }
    display.print(lastPulse); 
    display.print(" BPM"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }

  // Blood Oxygen Allocation
  display.setCursor(0, 24);
  display.print("Blood SpO2: ");
  if (bioHubReady) { 
    if (body.oxygen != 50) {
      lastSpO2 = body.oxygen;
    }
    display.print(lastSpO2); 
    display.print(" %"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }

  // Temperature Allocation
  display.setCursor(0, 36);
  display.print("Body Temp:  ");
  if (tempSensorReady) { 
    if (globalBodyTemp != 0) {
      lastTemp = globalBodyTemp;
    }
    display.print(lastTemp, 1); 
    display.print(" C"); 
  } 
  else { 
    display.print("SENSOR ERR"); 
  }

  // display time until device enters IDLE mode
  display.setCursor(0, 48);
  display.print("Idle in: ");
  uint32_t idleMs = gate0_msUntilDisarmed();
  if (idleMs > 1000) {
    display.print((idleMs + 999) / 1000); // round up to seconds
    display.print("s");
  } else {
    display.print("soon");
  }
}


// draw the idle screen when no vitals are available (e.g. during startup)
void drawIdleScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("Elderly Crutch Local");
  display.drawFastHLine(0, 10, 128, WHITE);

  display.setCursor(0, 28);
  display.print("IDLE Mode...");

  display.setCursor(20, 28);
  int touch = touchRead(TOUCH_PIN);
  display.print("Touch: ");
  display.print(touch);

  // display time until device is armed (if currently in the grace period after putting arm on the crutch)
  display.setCursor(0, 48);
  display.print("Armed in: ");
  uint32_t armedMs = gate0_msUntilArmed();
  if (armedMs > 0) {
    display.print((armedMs + 999) / 1000); // round up to seconds
    display.print("s");
  } else {
    display.print("soon");
  }
}



// ── Draw the fall-detection countdown screen ──────────────────
// Shown during Stage 5 only. The wearer sees a countdown and a
// prompt to press the panic button to cancel if they are OK.
void drawFallCountdownScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println("FALL DETECTED");
  display.drawFastHLine(0, 10, 128, WHITE);

  uint32_t remainingMs = fallCountdownRemainingMs();
  uint32_t remainingS  = (remainingMs + 999) / 1000;  // round up

  display.setTextSize(2);
  display.setCursor(20, 18);
  display.print("T-");
  display.print(remainingS);
  display.print("s");

  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("PRESS BUTTON TO CANCEL");
}

// ── Task: UI, Buttons, Alarm & Screen Painter (Every 50ms) ────
void updateDisplayAndStates() {
  // Handle screen rotation
  updateScreenMirror();
  
  // Handle alarm state and button logic
  updateAlarmState();

  // Paint the display
  display.clearDisplay();

  // Priority: alarm > fall countdown > vitals.
  // The alarm screen is reached either via manual panic press
  // or via Stage 5 expiring (which sets alarmActive directly).
  if (isAlarmActive()) {
    drawAlarmScreen();
  }
  else if (getFallStage() == FALL_STAGE5_COUNTDOWN) {
    drawFallCountdownScreen();
  }
  else if (!gate0_isArmed()) {
    drawIdleScreen();
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
