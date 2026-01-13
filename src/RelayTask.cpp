#include "RelayTask.h"
#include "Vent.h"
#include "Config.h"
#include <Arduino.h>

void RelayTask::process(Vent &vent, unsigned long now) {
  if (!active) return;

  // Initial flap pulse after flap opening time
  if (shouldPulseRE4 && !re4Triggered && !re4Pulsing && (now - startTime >= FLAP_OPENING_TIME)) {
    vent.startPulse(now);
  }

  // Scheduled/queued pulses
  if (nextPulseTime != 0 && now >= nextPulseTime) {
    // request RE4 pulse (Vent will accept or we will retry later)
    if (vent.startPulse(now)) {
      // on success, schedule next occurrences depending on task type
      if (!autoOff) {
        // re3 periodic: keep scheduling while active
        nextPulseTime = now + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
      } else {
        // consume one queued press
        if (pendingRE4 > 0) pendingRE4--;
        if (pendingRE4 > 0) {
          nextPulseTime = now + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
        } else {
          nextPulseTime = 0;
        }
      }
    }
  }

  // Auto-off after run time (for tasks with autoOff)
  if (autoOff && (now - startTime >= CENTRAL_FAN_RUN_TIME)) {
    // Keep relay ON while there are pending RE4 pulses or a scheduled nextPulse or an ongoing RE4 pulse.
    if (pendingRE4 == 0 && nextPulseTime == 0 && !re4Pulsing) {
      stop();
      #ifdef DEBUG
        Serial.print("Relay OFF: ");
        Serial.println(name);
      #endif
    } else {
      #ifdef DEBUG
        Serial.print("Relay remains ON after run time (pending=");
        Serial.print(pendingRE4);
        Serial.print(", nextPulse=");
        Serial.print(nextPulseTime);
        Serial.print(", re4Pulsing=");
        Serial.print(re4Pulsing);
        Serial.print(") - ");
        Serial.println(name);
      #endif
      // ensure we continue to schedule pulses: if there are pending requests but no nextPulseTime, schedule one
      if (pendingRE4 > 0 && nextPulseTime == 0) {
        nextPulseTime = now + CENTRAL_FAN_WAIT_TIME; // schedule shortly after run end (wait time ensures margin)
      }
    }
  }
}