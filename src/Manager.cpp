#include "Manager.h"
#include "Config.h"
#include <Arduino.h>

Manager::Manager(uint8_t re1Pin, uint8_t re2Pin, uint8_t re3Pin, uint8_t re4Pin,
                 uint8_t sw1Pin_, uint8_t sw2Pin_, uint8_t acsPin_)
    : re1("RE1", re1Pin), re2("RE2", re2Pin), re3("RE3", re3Pin),
      vent(re4Pin), sw1Pin(sw1Pin_), sw2Pin(sw2Pin_), acsPin(acsPin_)
{
  // re3 should remain until current drops
  re3.autoOff = false;
}

void Manager::begin()
{
  pinMode(sw1Pin, INPUT_PULLUP);
  pinMode(sw2Pin, INPUT_PULLUP);
  pinMode(re1.pin, OUTPUT);
  pinMode(re2.pin, OUTPUT);
  pinMode(re3.pin, OUTPUT);
  vent.init();

  digitalWrite(re1.pin, HIGH);
  digitalWrite(re2.pin, HIGH);
  digitalWrite(re3.pin, HIGH);

#if BME_ENABLED
  if (!bme.begin(0x76))
  { // try 0x76 first
    if (!bme.begin(0x77))
    { // fallback to 0x77
      DBG_PRINTLN("BME280 not found at 0x76 or 0x77. Check wiring.");
      while (1)
        delay(1000);
    }
  }
  DBG_PRINTLN("BME280 initialized");
#else
  DBG_PRINTLN("BME disabled in Config.h");
#endif
}

void Manager::turnOffAll()
{
  re1.stop();
  re2.stop();
  DBG_PRINTLN("All flaps turned OFF due to long switch hold.");
}

void Manager::handleButtons(unsigned long now)
{
  if (now - lastButtonsRead <= BUTTON_DEBOUNCE)
    return;
  lastButtonsRead = now;
  bool sw1 = digitalRead(sw1Pin);
  bool sw2 = digitalRead(sw2Pin);

  // edge: press
  if (sw1Prev == HIGH && sw1 == LOW)
  {
    DBG_PRINTLN("Switch pressed: SW1");
    if (!re1.active)
    {
      re1.begin(now);
    }
    else
    {
      re1.pendingRE4++;
      DBG_PRINT("Queued RE4 pulse for: ");
      DBG_PRINTLN(re1.name);
    }
  }
  if (sw2Prev == HIGH && sw2 == LOW)
  {
    DBG_PRINTLN("Switch pressed: SW2");
    if (!re2.active)
    {
      re2.begin(now);
    }
    else
    {
      re2.pendingRE4++;
      DBG_PRINT("Queued RE4 pulse for: ");
      DBG_PRINTLN(re2.name);
    }
  }

  // non-blocking hold detection (3s)
  if (sw1 == LOW)
  {
    if (sw1HoldStart == 0)
      sw1HoldStart = now;
    if (!sw1HoldHandled && (now - sw1HoldStart > 3000))
    {
      DBG_PRINTLN("Switch held: SW1 - turning off re1");
      re1.stop();
      sw1HoldHandled = true;
    }
  }
  else
  {
    sw1HoldStart = 0;
    sw1HoldHandled = false;
  }

  if (sw2 == LOW)
  {
    if (sw2HoldStart == 0)
      sw2HoldStart = now;
    if (!sw2HoldHandled && (now - sw2HoldStart > 3000))
    {
      DBG_PRINTLN("Switch held: SW2 - turning off re2");
      re2.stop();
      sw2HoldHandled = true;
    }
  }
  else
  {
    sw2HoldStart = 0;
    sw2HoldHandled = false;
  }

  sw1Prev = sw1;
  sw2Prev = sw2;
}

void Manager::handleHumidity(unsigned long now)
{
  if (now - lastHumidityRead <= HUMIDITY_READOUT_PERIOD)
    return;
  lastHumidityRead = now;
  float humidity = 0;
  // protect against missing BME
#if BME_ENABLED
  humidity = bme.readHumidity();
  DBG_PRINT("Humidity: ");
  DBG_PRINTLN(humidity);
  if (humidity > HUMIDITY_THRESHOLD)
  {
    if (!re1.active)
    {
      DBG_PRINTLN("Humidity triggered RE1");
      re1.begin(now);
    }
  }
#else
  // BME disabled: skip humidity reads
  (void)now;
#endif
}

void Manager::handleCurrent(unsigned long now)
{
  if (now - lastCurrentRead <= CURRENT_READOUT_PERIOD)
    return;
  lastCurrentRead = now;
  int sensorValue = analogRead(acsPin);

  // open flap when hood-on is detected
  if (sensorValue > CURRENT_THRESHOLD)
  {
    if (!re3.active)
    {
      DBG_PRINTLN("Current triggered RE3");
      re3.begin(now); // re3 stays active until current drops (autoOff=false)
    }
  }

  if (re3.active)
  {
    // initialize window on first sample
    if (currentWindowStart == 0)
    {
      currentWindowStart = now;
      currentWindowMax = -1;
    }
    // New max in current window?
    if (sensorValue > currentWindowMax)
    {
      currentWindowMax = sensorValue;
    }

    // If window expired, reset and start new window
    if (now - currentWindowStart >= CURRENT_WINDOW_MS)
    {
      currentWindowStart = now;
      if (currentWindowMax <= CURRENT_THRESHOLD)
      { // no more current detection
        DBG_PRINTLN("Current released RE3");
        re3.stop();
      }
      currentWindowMax = -1;
      Serial.println("Window reset - starting new max detection");
    }
  }
}

void Manager::update()
{
  unsigned long now = millis();

  vent.update(now);    // handle RE4 completion
  handleButtons(now);  // poll buttons
  handleHumidity(now); // humidity periodic
  handleCurrent(now);  // current periodic

  // process each relay (schedules pulses, auto-off)
  re1.process(vent, now);
  re2.process(vent, now);
  re3.process(vent, now);
}
