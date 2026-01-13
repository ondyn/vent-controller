#include "RelayTask.h"
#include "Vent.h"
#include "Config.h"
#include <Arduino.h>

void RelayTask::process(Vent &vent, unsigned long now)
{
  if (!active)
    return;

  // Pulse ventilation after flap opening time
  if (pendingRE4 > 0 && (now - startTime >= FLAP_OPENING_TIME))
  {
    pendingRE4--;
    vent.startPulse(now);
  }

  // Auto-off after run time (for tasks with autoOff)
  if (autoOff && (now - startTime >= CENTRAL_FAN_RUN_TIME))
  {
    stop();
    #ifdef DEBUG
        Serial.print("Relay OFF: ");
        Serial.println(name);
    #endif
  } else if (now - startTime >= CENTRAL_FAN_RUN_TIME){
    startTime = now;
    vent.startPulse(now);
  }
}