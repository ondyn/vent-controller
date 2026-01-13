#ifndef VENT_H
#define VENT_H

#include <Arduino.h>
#include "RelayTask.h"

// local debug helpers (no dependency on DEBUG symbol existing at compile-unit scope)
#ifdef DEBUG
  #define VDBG_PRINT(x) Serial.print(x)
  #define VDBG_PRINTLN(x) Serial.println(x)
#else
  #define VDBG_PRINT(x)
  #define VDBG_PRINTLN(x)
#endif

class Vent {
  uint8_t re4Pin;
  bool pulsing = false;
  unsigned long pulseStart = 0;
public:
  Vent(uint8_t re4Pin_) : re4Pin(re4Pin_) {}
  void begin() {
    pinMode(re4Pin, OUTPUT);
    digitalWrite(re4Pin, HIGH);
    pulsing = false;
    pulseStart = 0;
  }

  // Start a RE4 pulse (1s) if not already pulsing.
  // Returns true if the pulse was started (otherwise ignored).
  bool startPulse(unsigned long now) {
    if (pulsing) return false;
    digitalWrite(re4Pin, LOW);
    pulsing = true;
    pulseStart = now;
    VDBG_PRINT("Started RE4 pulse");
    VDBG_PRINTLN("");
    return true;
  }

  // Call regularly to finish RE4 pulses
  void update(unsigned long now) {
    if (!pulsing) return;
    if (now - pulseStart >= 1000) {
      digitalWrite(re4Pin, HIGH);
      pulsing = false;
      VDBG_PRINT("Completed RE4 pulse");
      VDBG_PRINTLN("");
    }
  }

  // Ensure RE4 is idle (HIGH). no force-stop of active pulse.
  void ensureIdle() {
    digitalWrite(re4Pin, HIGH);
  }
};

#undef VDBG_PRINT
#undef VDBG_PRINTLN

#endif // VENT_H