#ifndef VENT_H
#define VENT_H

#include <Arduino.h>
#include "RelayTask.h"
#include "Config.h"

class Vent
{
  uint8_t re4Pin;
  bool pulsing = false;
  unsigned long pulseStart = 0;

public:
  Vent(uint8_t re4Pin_) : re4Pin(re4Pin_) {}

  void init()
  {
    pinMode(re4Pin, OUTPUT);
    digitalWrite(re4Pin, HIGH);
    pulsing = false;
    pulseStart = 0;
  }

  // Start a RE4 pulse (1s) if not already pulsing.
  // Returns true if the pulse was started (otherwise ignored).
  bool startPulse(unsigned long now)
  {
    if (pulsing)
      return false;
    digitalWrite(re4Pin, LOW);
    pulsing = true;
    pulseStart = now;
    DBG_PRINTLN("Started vent pulse");
    return true;
  }

  // Call regularly to finish RE4 pulse
  void update(unsigned long now)
  {
    if (!pulsing)
      return;
    if (now - pulseStart >= 1000)
    {
      digitalWrite(re4Pin, HIGH);
      pulsing = false;
      DBG_PRINTLN("Completed vent pulse");
    }
  }
};

#endif // VENT_H