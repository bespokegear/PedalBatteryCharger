#pragma once

#include <Arduino.h>

// ********** Hardware information********************************************
// These constants won't change.  They're used to give names to the pins used:
#define ANALOGCURRENT   A3  // I read in on A3
#define ANALOGVOLTAGE   A0  // V read in on A0
#define DEVICEID        "AA"

#define DEBUG           0   // If set to 1 the use debug if 0 the no debug 
// Analog read settings
#define AVERAGECOUNTER  500    // The number of samples to average over
#define ANALOGREF       3.24   //The analog voltage reference (external 3.3V)

//Voltage Read Settings
#define RESISTORTOP     98.0   // R1 value in kohm
#define RESISTORBOTTOM  10.0    // R2 value in kohm

//Current Read Settings
#define VSUPPLY         4.26  //The volatge supply to the current sensor
//#define VSUPPLY         4.6  //The volatge supply to the current sensor with USB
#define CURRENTCONV     40.0   //The conversion of mV per A at 5V from Current sensor datasheet

// Button feel settings
#define DEBOUNCED_BUTTON_THRESHOLD      5
#define DEBOUNCED_BUTTON_DELAY          5
#define DEBOUNCED_BUTTON_HELD_MS        300
#define DEBOUNCED_BUTTON_RPT_INITIAL_MS 500
#define DEBOUNCED_BUTTON_RPT_MS         300

// Button parameters
//#define RESET_BUTTON_PIN                9

#define DISPLAYUPDATEMS                 1000 // mS between each display and button press check update
