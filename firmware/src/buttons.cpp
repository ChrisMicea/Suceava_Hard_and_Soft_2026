// #include "buttons.h"
// #include <queue>

// std::queue<int> touchReadings; // Queue to hold recent touch readings for debouncing

// uint32_t lastPanicButtonPressTime = 0; // For debouncing the panic button

// void setupButtons() {
//     pinMode(BUTTON_PIN, INPUT);
// }

// // ~3.3V when panic pressed, ~1.6 when other button is pressed, 0 when no buttons are pressed
// bool isPanicButtonPressed() {

//     uint32_t now = millis();
//     if (now - lastPanicButtonPressTime < 200) {
//         return false; // Ignore if within debounce period
//     }

//     lastPanicButtonPressTime = now;

//     // 4000 = analogRead() value read
//     int buttonInput = analogRead(BUTTON_PIN);
//     // Serial.printf("3.3 V Button Read: %d\n", buttonInput); // Debug print to monitor button readings
//     delay(1);
//     // return false;
//     return buttonInput > 3000; // Adjust threshold as needed based on actual readings
// }

// bool isInDebounce() {
//     return (millis() - lastPanicButtonPressTime) < 200; // 200ms debounce period
// }

// bool isOtherButtonPressed() {
//     // 1800 = analogRead() value read
//     int buttonInput = analogRead(BUTTON_PIN);
//     // Serial.printf("1.6 V Button Read: %d\n", buttonInput); // Debug print to monitor button readings
//     delay(1);
//     // return false;
//     return buttonInput > 1500 && buttonInput <= 3000; // Adjust threshold as needed based on actual readings
// }

// bool isTouchPressed() {
//     // ESP32 touch: lower value = more capacitance = finger present
//     // Bare aluminum foil pad typically reads ~15–40 untouched, ~5–12 touched
//     // Calibrate TOUCH_THRESHOLD to your specific pad size
//     int touchInput = touchRead(TOUCH_PIN);
//     if (touchReadings.size() >= 10) { // Keep only the last 10 readings for debouncing
//         touchReadings.pop();
//     }
//     touchReadings.push(touchInput);
//     Serial.printf("Touch Read: %d\n", touchInput); // Debug print to monitor touch readings
//     delay(1);
//     static const int TOUCH_THRESHOLD = 45;

//     int sum = 0;
//     std::queue<int> tempQueue = touchReadings; // Copy to a temp queue
//     while (!tempQueue.empty()) {
//         sum += tempQueue.front();
//         tempQueue.pop();
//     }

//     if (sum >= TOUCH_THRESHOLD) {
//         return false; // Not touched
//     }
//     else {
//         return true; // Touched
//     }
// }


#include "buttons.h"
#include <queue>

std::queue<int> touchReadings; // Queue to hold recent touch readings for debouncing

// ── Panic button state ────────────────────────────────────────
// `lastPanicEdgeTime` records the timestamp of the most recent
// confirmed *state change* (rising or falling edge) of the panic
// button. The debounce gate blocks new edges from registering for
// 200 ms after the last accepted edge, but it NEVER suppresses the
// reported steady state.
//
// Why this matters: the previous implementation refreshed the
// timestamp on every call (even when the pin read low) and returned
// `false` whenever the gate was open, which produced a one-shot
// pulse instead of a steady level. Higher layers that needed to see
// "the button is still held down right now" (alarm hold-to-reset,
// fall countdown cancel) couldn't reliably do so, and edge-detect
// logic upstream got fed phantom transitions.
//
// Current contract:
//   - Returns the current debounced state of the button (true while
//     held, false while released), not a one-shot pulse.
//   - The internal state only flips after the new raw reading
//     differs from the current debounced state AND the previous
//     accepted edge was at least 200 ms ago.
static uint32_t lastPanicEdgeTime = 0;
static bool     panicDebouncedState = false;

static constexpr uint32_t PANIC_DEBOUNCE_MS = 200;

void setupButtons() {
    pinMode(BUTTON_PIN, INPUT);
}

// ~3.3V when panic pressed, ~1.6 when other button is pressed, 0 when no buttons are pressed
bool isPanicButtonPressed() {
    int buttonInput = analogRead(BUTTON_PIN);
    bool rawPressed = (buttonInput > 3000);

    uint32_t now = millis();
    if (rawPressed != panicDebouncedState &&
        (now - lastPanicEdgeTime) >= PANIC_DEBOUNCE_MS) {
        // Accept the edge.
        panicDebouncedState = rawPressed;
        lastPanicEdgeTime   = now;
    }
    return panicDebouncedState;
}

bool isInDebounce() {
    return (millis() - lastPanicEdgeTime) < PANIC_DEBOUNCE_MS;
}

bool isOtherButtonPressed() {
    // 1800 = analogRead() value read
    int buttonInput = analogRead(BUTTON_PIN);
    // Serial.printf("1.6 V Button Read: %d\n", buttonInput); // Debug print to monitor button readings
    delay(1);
    // return false;
    return buttonInput > 1500 && buttonInput <= 3000; // Adjust threshold as needed based on actual readings
}

bool isTouchPressed() {
    // ESP32 touch: lower value = more capacitance = finger present
    // Bare aluminum foil pad typically reads ~15–40 untouched, ~5–12 touched
    // Calibrate TOUCH_THRESHOLD to your specific pad size
    int touchInput = touchRead(TOUCH_PIN);
    if (touchReadings.size() >= 10) { // Keep only the last 10 readings for debouncing
        touchReadings.pop();
    }
    touchReadings.push(touchInput);
    Serial.printf("Touch Read: %d\n", touchInput); // Debug print to monitor touch readings
    delay(1);
    static const int TOUCH_THRESHOLD = 45;

    int sum = 0;
    std::queue<int> tempQueue = touchReadings; // Copy to a temp queue
    while (!tempQueue.empty()) {
        sum += tempQueue.front();
        tempQueue.pop();
    }

    if (sum >= TOUCH_THRESHOLD) {
        return false; // Not touched
    }
    else {
        return true; // Touched
    }
}