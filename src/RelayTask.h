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
  uint8_t pendingRE4 = 0;
  bool autoOff = true;

  RelayTask(const char *n = "", uint8_t p = 0) : name(n), pin(p) {}

  void begin(unsigned long now) {
    digitalWrite(pin, LOW);
    active = true;
    startTime = now;
    pendingRE4 = 1;
  }

  void stop() {
    digitalWrite(pin, HIGH);
    active = false;
    pendingRE4 = 0;
  }

  // Called regularly from Manager::update()
  void process(Vent &vent, unsigned long now);

};

#endif // RELAY_TASK_H