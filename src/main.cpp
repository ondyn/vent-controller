#include "Config.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Arduino.h>

#include "Manager.h"

Adafruit_BME280 bme;
Manager manager(RE1_PIN, RE2_PIN, RE3_PIN, RE4_PIN, SW1_PIN, SW2_PIN, ACS712_PIN, bme);

void setup() {
  manager.begin();
}

void loop() {
  manager.update();
  delay(50);
}