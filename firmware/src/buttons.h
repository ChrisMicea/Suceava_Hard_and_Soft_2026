#pragma once

#include "hardware_config.h"
#include <Arduino.h>

void setupButtons();
bool isPanicButtonPressed();
bool isOtherButtonPressed();
bool isTouchPressed();
bool isInDebounce();
