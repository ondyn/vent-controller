#ifndef VENT_CONFIG_H
#define VENT_CONFIG_H

#include <Arduino.h> // provide uint8_t, A0, etc.

// Enable/disable debug output across all TUs
#define DEBUG 1

// Timing / thresholds
constexpr unsigned long FLAP_OPENING_TIME       = 5000UL;
constexpr unsigned long CENTRAL_FAN_RUN_TIME    = 120000UL;
constexpr unsigned long CENTRAL_FAN_WAIT_TIME   = 10000UL;

constexpr unsigned long HUMIDITY_READOUT_PERIOD = 10000UL;
constexpr unsigned long CURRENT_READOUT_PERIOD  = 5000UL;

constexpr float HUMIDITY_THRESHOLD = 70.0f;
constexpr float CURRENT_THRESHOLD  = 500.0f; // mA

// Pins
constexpr uint8_t SW1_PIN  = 2;
constexpr uint8_t SW2_PIN  = 3;
constexpr uint8_t RE1_PIN  = 4;
constexpr uint8_t RE2_PIN  = 5;
constexpr uint8_t RE3_PIN  = 6;
constexpr uint8_t RE4_PIN  = 7;
constexpr uint8_t ACS712_PIN = A0;

#endif // VENT_CONFIG_H