#include "Config.h"
#include <Arduino.h>

#if BME_ENABLED
  #include <Adafruit_BME280.h>
  #include <Wire.h>
  #include <Adafruit_Sensor.h>
#endif

#include "Manager.h"

Manager manager(RE1_PIN, RE2_PIN, RE3_PIN, RE4_PIN, SW1_PIN, SW2_PIN, ACS712_PIN);

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("Main Setup");
  manager.begin();
  Serial.println("Manager started");
}

void loop() {
  manager.update();
}