#ifndef MANAGER_H
#define MANAGER_H

#include <Arduino.h>
#include "Vent.h"
#include "RelayTask.h"
#include "Config.h"

#if BME_ENABLED
  #include <Wire.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BME280.h>
#endif

class Manager {
public:
  // bmePtr may be null when BME_ENABLED == 0
  Manager(uint8_t re1Pin, uint8_t re2Pin, uint8_t re3Pin, uint8_t re4Pin,
          uint8_t sw1Pin, uint8_t sw2Pin, uint8_t acsPin);

  void begin();
  void update();

private:
  RelayTask re1;
  RelayTask re2;
  RelayTask re3;
  Vent vent;

  uint8_t sw1Pin, sw2Pin, acsPin;
#if BME_ENABLED
  Adafruit_BME280 bme;
#endif

  unsigned long lastHumidityRead = 0;
  unsigned long lastCurrentRead = 0;
  unsigned long lastButtonsRead = 0;
  unsigned long sw1HoldStart = 0;
  unsigned long sw2HoldStart = 0;
  bool sw1HoldHandled = false;
  bool sw2HoldHandled = false;
  bool sw1Prev = HIGH;
  bool sw2Prev = HIGH;

  unsigned long lastCurrentMillis = 0;
  unsigned long currentWindowStart = 0;
  int currentWindowMax = -1;

  void handleButtons(unsigned long now);
  void handleHumidity(unsigned long now);
  void handleCurrent(unsigned long now);
  void turnOffAll();
};

#endif // MANAGER_H