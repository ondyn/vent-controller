#include <Wire.h>
#include <Adafruit_BME280.h>

#define SW1_PIN 2
#define SW2_PIN 3

#define RE1_PIN 4
#define RE2_PIN 5
#define RE3_PIN 6
#define RE4_PIN 7

#define ACS712_PIN A0

#define FLAP_OPENING_TIME 5000
#define CENTRAL_FAN_RUN_TIME 120000
#define HUMIDITY_READOUT_PERIOD 10000
#define CURRENT_READOUT_PERIOD 5000
#define HUMIDITY_THRESHOLD 70.0
#define CURRENT_THRESHOLD 500

bool DEBUG = true; // Set to false to disable debug output

Adafruit_BME280 bme;

unsigned long lastHumidityRead = 0;
unsigned long lastCurrentRead = 0;
unsigned long sw1HoldStart = 0;
unsigned long sw2HoldStart = 0;

struct RelayTask {
  const char *name;
  bool active = false;
  unsigned long startTime = 0;
  bool re4Triggered = false;
  bool re4Pulsing = false;
  unsigned long re4PulseStart = 0;
  uint8_t pendingRE4 = 0;
  unsigned long nextPulseTime = 0;
  bool autoOff = true; // re3 can set false if needed
  RelayTask(const char *relayName = "") : name(relayName) {}
};

RelayTask re1Task("RE1");
RelayTask re2Task("RE2");
RelayTask re3Task("RE3");

// Debug helper (single function, prints with/without newline)
inline void dbgPrint(const char *msg, bool nl = true) {
  if (!DEBUG) return;
  if (nl) Serial.println(msg); else Serial.print(msg);
}

void turnOffAllRelays() {
  digitalWrite(RE1_PIN, HIGH);
  digitalWrite(RE2_PIN, HIGH);
  digitalWrite(RE3_PIN, HIGH);
  digitalWrite(RE4_PIN, HIGH);
  re1Task.active = false;
  re1Task.re4Pulsing = false;
  re1Task.re4Triggered = false;
  re1Task.pendingRE4 = 0;
  re1Task.nextPulseTime = 0;
  re2Task.active = false;
  re2Task.re4Pulsing = false;
  re2Task.re4Triggered = false;
  re2Task.pendingRE4 = 0;
  re2Task.nextPulseTime = 0;
  re3Task.active = false;
  re3Task.re4Pulsing = false;
  re3Task.re4Triggered = false;
  re3Task.pendingRE4 = 0;
  re3Task.nextPulseTime = 0;
  dbgPrint("All RE OFF - long SW");
}

// Helper to start relay task
void startRelayTask(RelayTask &task, int relayPin, bool triggerRE4) {
  digitalWrite(relayPin, LOW); // active low relay
  task.active = true;
  task.startTime = millis();
  task.re4Triggered = !triggerRE4; // if we want RE4 pulse, mark as not yet triggered
  task.re4Pulsing = false;
  task.re4PulseStart = 0;
  task.pendingRE4 = 0;
  task.nextPulseTime = 0;
  dbgPrint("RE ON: ", false);
  dbgPrint(task.name);
}

// Helper to process relay task (keeps original behavior)
void processRelayTask(RelayTask &task, int relayPin) {
  if (!task.active) return;
  unsigned long now = millis();

  // After FLAP_OPENING_TIME, trigger RE4 for 1s (if not already triggered)
  if (!task.re4Triggered && (now - task.startTime >= FLAP_OPENING_TIME)) {
    digitalWrite(RE4_PIN, LOW);
    delay(1000); // original behavior: 1s pulse (blocking)
    digitalWrite(RE4_PIN, HIGH);
    task.re4Triggered = true;
    dbgPrint("RE4 ON by: ", false);
    dbgPrint(task.name);
  }

  // handle scheduled extra pulses (queued while active)
  if (task.nextPulseTime != 0 && now >= task.nextPulseTime) {
    // pulse RE4 once (blocking as before)
    digitalWrite(RE4_PIN, LOW);
    delay(1000);
    digitalWrite(RE4_PIN, HIGH);
    dbgPrint("RE4 queued pulse by: ", false);
    dbgPrint(task.name);
    if (task.pendingRE4 > 0) {
      task.pendingRE4--;
      if (task.pendingRE4 > 0) {
        task.nextPulseTime = now + CENTRAL_FAN_RUN_TIME + 10000; // wait before next
      } else {
        task.nextPulseTime = 0;
      }
    } else {
      task.nextPulseTime = 0;
    }
  }

  // Auto-off only if allowed (re1/re2). re3 can be configured to stay on via autoOff=false
  if (task.autoOff && (now - task.startTime >= CENTRAL_FAN_RUN_TIME)) {
    digitalWrite(relayPin, HIGH);
    task.active = false;
    task.re4Pulsing = false;
    task.re4Triggered = false;
    task.pendingRE4 = 0;
    task.nextPulseTime = 0;
    dbgPrint("RE OFF ", false);
    dbgPrint(task.name);
  }
}

void setup() {
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(RE1_PIN, OUTPUT);
  pinMode(RE2_PIN, OUTPUT);
  pinMode(RE3_PIN, OUTPUT);
  pinMode(RE4_PIN, OUTPUT);

  digitalWrite(RE1_PIN, HIGH);
  digitalWrite(RE2_PIN, HIGH);
  digitalWrite(RE3_PIN, HIGH);
  digitalWrite(RE4_PIN, HIGH);

  Serial.begin(9600);
  bme.begin(0x76); // Check your BME280 I2C address
  // keep re3 autoOff=false if you want it controlled by current sensor logic
  re3Task.autoOff = false;
}

void loop() {
  // Button handling
  static bool sw1Prev = HIGH, sw2Prev = HIGH;
  bool sw1 = digitalRead(SW1_PIN);
  bool sw2 = digitalRead(SW2_PIN);
  unsigned long now = millis();

  // SW1 pressed
  if (sw1Prev == HIGH && sw1 == LOW) {
    dbgPrint("SW1 pressed");
    if (!re1Task.active) {
      startRelayTask(re1Task, RE1_PIN, true);
      re1Task.nextPulseTime = re1Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
    } else {
      re1Task.pendingRE4++;
      if (re1Task.nextPulseTime == 0) re1Task.nextPulseTime = re1Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
      dbgPrint("Queued RE4 for: ", false);
      dbgPrint(re1Task.name);
    }
  }

  // SW2 pressed
  if (sw2Prev == HIGH && sw2 == LOW) {
    dbgPrint("SW2 pressed");
    if (!re2Task.active) {
      startRelayTask(re2Task, RE2_PIN, true);
      re2Task.nextPulseTime = re2Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
    } else {
      re2Task.pendingRE4++;
      if (re2Task.nextPulseTime == 0) re2Task.nextPulseTime = re2Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
      dbgPrint("Queued RE4 for: ", false);
      dbgPrint(re2Task.name);
    }
  }

  // Non-blocking switch hold detection (3s)
  if (sw1 == LOW) {
    if (sw1HoldStart == 0) sw1HoldStart = now;
    if ((now - sw1HoldStart) > 3000) {
      turnOffAllRelays();
      // wait until switch is released to avoid repeated triggers (small blocking)
      while (digitalRead(SW1_PIN) == LOW) delay(10);
      sw1HoldStart = 0;
      sw2HoldStart = 0;
      sw1Prev = HIGH;
      sw2Prev = HIGH;
      return;
    }
  } else {
    sw1HoldStart = 0;
  }

  if (sw2 == LOW) {
    if (sw2HoldStart == 0) sw2HoldStart = now;
    if ((now - sw2HoldStart) > 3000) {
      turnOffAllRelays();
      while (digitalRead(SW2_PIN) == LOW) delay(10);
      sw1HoldStart = 0;
      sw2HoldStart = 0;
      sw1Prev = HIGH;
      sw2Prev = HIGH;
      return;
    }
  } else {
    sw2HoldStart = 0;
  }

  sw1Prev = sw1;
  sw2Prev = sw2;

  // Humidity sensor
  if (now - lastHumidityRead > HUMIDITY_READOUT_PERIOD) {
    lastHumidityRead = now;
    float humidity = bme.readHumidity();
    if (DEBUG) { Serial.print("Hum: "); Serial.println(humidity); }
    if (humidity > HUMIDITY_THRESHOLD) {
      if (!re1Task.active) {
        dbgPrint("Hum triggered RE1");
        startRelayTask(re1Task, RE1_PIN, true);
        re1Task.nextPulseTime = re1Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
      } else {
        re1Task.pendingRE4++;
        if (re1Task.nextPulseTime == 0) re1Task.nextPulseTime = re1Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
      }
    }
  }

  // Current sensor
  if (now - lastCurrentRead > CURRENT_READOUT_PERIOD) {
    lastCurrentRead = now;
    int sensorValue = analogRead(ACS712_PIN);
    float voltage = sensorValue * (5.0 / 1023.0);
    float current = (voltage - 2.5) / 0.185;
    current = abs(current);
    if (DEBUG) { Serial.print("Cur: "); Serial.println(current * 1000); }
    if (current * 1000 > CURRENT_THRESHOLD) {
      if (!re3Task.active) {
        dbgPrint("Cur triggered RE3");
        startRelayTask(re3Task, RE3_PIN, true);
        re3Task.nextPulseTime = re3Task.startTime + CENTRAL_FAN_RUN_TIME + 10000;
      }
      // re3Task.autoOff is false so it will be kept active until current drops
    } else {
      if (re3Task.active && !re3Task.autoOff) {
        digitalWrite(RE3_PIN, HIGH);
        re3Task.active = false;
        re3Task.pendingRE4 = 0;
        re3Task.nextPulseTime = 0;
        dbgPrint("RE3 OFF (current low)");
      }
    }
  }

  // Process relay tasks
  processRelayTask(re1Task, RE1_PIN);
  processRelayTask(re2Task, RE2_PIN);
  processRelayTask(re3Task, RE3_PIN);

  delay(50);
}