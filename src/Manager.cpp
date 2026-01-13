#include "Manager.h"
#include "Config.h"
#include <Arduino.h>

// local debug helpers (no dependency on DEBUG symbol existing in this TU)
#ifdef DEBUG
  #define MDBG_PRINT(x) Serial.print(x)
  #define MDBG_PRINTLN(x) Serial.println(x)
#else
  #define MDBG_PRINT(x)
  #define MDBG_PRINTLN(x)
#endif

Manager::Manager(uint8_t re1Pin, uint8_t re2Pin, uint8_t re3Pin, uint8_t re4Pin,
                 uint8_t sw1Pin_, uint8_t sw2Pin_, uint8_t acsPin_, Adafruit_BME280 &bmeRef)
  : re1("RE1", re1Pin), re2("RE2", re2Pin), re3("RE3", re3Pin),
    vent(re4Pin), sw1Pin(sw1Pin_), sw2Pin(sw2Pin_), acsPin(acsPin_), bme(bmeRef)
{
  // re3 should remain until current drops
  re3.autoOff = false;
}

void Manager::begin() {
  pinMode(sw1Pin, INPUT_PULLUP);
  pinMode(sw2Pin, INPUT_PULLUP);
  pinMode(re1.pin, OUTPUT);
  pinMode(re2.pin, OUTPUT);
  pinMode(re3.pin, OUTPUT);
  vent.begin();

  digitalWrite(re1.pin, HIGH);
  digitalWrite(re2.pin, HIGH);
  digitalWrite(re3.pin, HIGH);

  Serial.begin(9600);
  if (!bme.begin(0x76)) {
    MDBG_PRINTLN("BME280 not found");
  } else {
    MDBG_PRINTLN("BME280 init");
  }
}

void Manager::turnOffAll() {
  re1.stop();
  re2.stop();
  re3.stop();
  // RE4 is independent; just ensure it's idle (do not force-stop an ongoing pulse)
  vent.ensureIdle();
  MDBG_PRINTLN("All relays turned OFF due to long switch hold.");
}

void Manager::handleButtons(unsigned long now) {
  bool sw1 = digitalRead(sw1Pin);
  bool sw2 = digitalRead(sw2Pin);

  // edge: press
  if (sw1Prev == HIGH && sw1 == LOW) {
    MDBG_PRINTLN("Switch pressed: SW1");
    if (!re1.active) {
      re1.begin(now, true);
      re1.nextPulseTime = re1.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
    } else {
      re1.pendingRE4++;
      if (re1.nextPulseTime == 0) re1.nextPulseTime = re1.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
      MDBG_PRINT("Queued RE4 pulse for: ");
      MDBG_PRINTLN(re1.name);
    }
  }
  if (sw2Prev == HIGH && sw2 == LOW) {
    MDBG_PRINTLN("Switch pressed: SW2");
    if (!re2.active) {
      re2.begin(now, true);
      re2.nextPulseTime = re2.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
    } else {
      re2.pendingRE4++;
      if (re2.nextPulseTime == 0) re2.nextPulseTime = re2.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
      MDBG_PRINT("Queued RE4 pulse for: ");
      MDBG_PRINTLN(re2.name);
    }
  }

  // non-blocking hold detection (3s)
  if (sw1 == LOW) {
    if (sw1HoldStart == 0) sw1HoldStart = now;
    if (!sw1HoldHandled && (now - sw1HoldStart > 3000)) {
      turnOffAll();
      sw1HoldHandled = true;
    }
  } else {
    sw1HoldStart = 0;
    sw1HoldHandled = false;
  }

  if (sw2 == LOW) {
    if (sw2HoldStart == 0) sw2HoldStart = now;
    if (!sw2HoldHandled && (now - sw2HoldStart > 3000)) {
      turnOffAll();
      sw2HoldHandled = true;
    }
  } else {
    sw2HoldStart = 0;
    sw2HoldHandled = false;
  }

  sw1Prev = sw1;
  sw2Prev = sw2;
}

void Manager::handleHumidity(unsigned long now) {
  if (now - lastHumidityRead <= HUMIDITY_READOUT_PERIOD) return;
  lastHumidityRead = now;
  float humidity = 0;
  // protect against missing BME
  if (bme.begin(0x76) || true) {
    humidity = bme.readHumidity();
  }
  MDBG_PRINT("Humidity: ");
  MDBG_PRINTLN(humidity);
  if (humidity > HUMIDITY_THRESHOLD) {
    if (!re1.active) {
      MDBG_PRINTLN("Humidity triggered RE1");
      re1.begin(now, true);
      re1.nextPulseTime = re1.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
    } else {
      re1.pendingRE4++;
      if (re1.nextPulseTime == 0) re1.nextPulseTime = re1.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
    }
  }
}

void Manager::handleCurrent(unsigned long now) {
  if (now - lastCurrentRead <= CURRENT_READOUT_PERIOD) return;
  lastCurrentRead = now;
  int sensorValue = analogRead(acsPin);
  float voltage = sensorValue * (5.0 / 1023.0);
  float current = fabs((voltage - 2.5) / 0.185f);
  MDBG_PRINT("Current (mA): ");
  MDBG_PRINTLN(current * 1000.0f);

  if (current * 1000.0f > CURRENT_THRESHOLD) {
    if (!re3.active) {
      MDBG_PRINTLN("Current triggered RE3");
      re3.begin(now, true);
      re3.nextPulseTime = re3.startTime + CENTRAL_FAN_RUN_TIME + CENTRAL_FAN_WAIT_TIME;
    }
    // re3 stays active until current drops (autoOff=false)
  } else {
    if (re3.active && !re3.autoOff) {
      re3.stop();
      MDBG_PRINTLN("Relay OFF due to current drop: RE3");
    }
  }
}

void Manager::update() {
  unsigned long now = millis();
  vent.update(now);            // handle RE4 completion
  handleButtons(now);          // poll buttons
  handleHumidity(now);         // humidity periodic
  handleCurrent(now);          // current periodic

  // process each relay (schedules pulses, auto-off)
  re1.process(vent, now);
  re2.process(vent, now);
  re3.process(vent, now);
}

#undef MDBG_PRINT
#undef MDBG_PRINTLN
