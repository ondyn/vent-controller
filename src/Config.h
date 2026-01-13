#ifndef VENT_CONFIG_H
#define VENT_CONFIG_H

#include <Arduino.h> // provide uint8_t, A0, etc.

// Enable/disable debug output across all TUs
#define DEBUG 1

#ifdef DEBUG
  #define DBG_PRINT(x) Serial.print(x)
  #define DBG_PRINTLN(x) Serial.println(x)
#else
  #define DBG_PRINT(x)
  #define DBG_PRINTLN(x)
#endif

// Enable/disable BME280 usage (1 = enabled, 0 = disabled)
#define BME_ENABLED 0

// Timing / thresholds
constexpr unsigned long FLAP_OPENING_TIME       = 5000UL;
constexpr unsigned long CENTRAL_FAN_RUN_TIME    = 120000UL;
constexpr unsigned long CENTRAL_FAN_WAIT_TIME   = 10000UL;

constexpr unsigned long HUMIDITY_READOUT_PERIOD = 10000UL;
constexpr unsigned long CURRENT_READOUT_PERIOD  = 25UL; // 25ms, NOT to be aligned to 50Hz~20ms
constexpr unsigned long CURRENT_WINDOW_MS       = 10000UL;
constexpr unsigned long BUTTON_DEBOUNCE         = 50UL;

constexpr float HUMIDITY_THRESHOLD = 70.0f;
// Hood measured max values:
// idle            517-519
// light ony       522-526
// vent + light    533-541
// vent only       527-530
constexpr int CURRENT_THRESHOLD  = 526;

// Pins
constexpr uint8_t SW1_PIN  = 2;
constexpr uint8_t SW2_PIN  = 3;
constexpr uint8_t RE1_PIN  = 4;
constexpr uint8_t RE2_PIN  = 5;
constexpr uint8_t RE3_PIN  = 6;
constexpr uint8_t RE4_PIN  = 7;
constexpr uint8_t ACS712_PIN = A0; //ACS712 5A

#endif // VENT_CONFIG_H