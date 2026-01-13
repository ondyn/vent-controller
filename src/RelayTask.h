#ifndef RELAY_TASK_H
#define RELAY_TASK_H

#include <Arduino.h>

class Vent; // forward

class RelayTask {
public:
  const char *name;
  uint8_t pin;
  bool active = false;
  unsigned long startTime = 0;
  bool re4Triggered = false;
  bool re4Pulsing = false;
  bool shouldPulseRE4 = false;
  uint8_t pendingRE4 = 0;
  unsigned long nextPulseTime = 0;
  bool autoOff = true;

  RelayTask(const char *n = "", uint8_t p = 0) : name(n), pin(p) {}

  void begin(unsigned long now, bool triggerRE4) {
    digitalWrite(pin, LOW);
    active = true;
    startTime = now;
    re4Triggered = !triggerRE4;
    re4Pulsing = false;
    shouldPulseRE4 = triggerRE4;
    pendingRE4 = 0;
    nextPulseTime = 0;
  }

  void stop() {
    digitalWrite(pin, HIGH);
    active = false;
    re4Pulsing = false;
    re4Triggered = false;
    shouldPulseRE4 = false;
    pendingRE4 = 0;
    nextPulseTime = 0;
  }

  // Called regularly from Manager::update()
  void process(Vent &vent, unsigned long now);

};

#endif // RELAY_TASK_H