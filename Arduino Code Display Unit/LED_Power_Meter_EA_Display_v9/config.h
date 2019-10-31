#pragma once

#include <Arduino.h>

// Button feel settings
#define DEBOUNCED_BUTTON_THRESHOLD      5
#define DEBOUNCED_BUTTON_DELAY          5
#define DEBOUNCED_BUTTON_HELD_MS        300
#define DEBOUNCED_BUTTON_RPT_INITIAL_MS 500
#define DEBOUNCED_BUTTON_RPT_MS         300

// Button parameters
#define RESET_BUTTON_PIN                9

#define DISPLAYUPDATEMS                 500 // mS between each display and button press check update
