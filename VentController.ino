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
  String name;
  bool active = false;
  unsigned long startTime = 0;
  bool re4Triggered = false;
  bool relayOn = false;
  RelayTask(String relayName = "") : name(relayName) {}
};

RelayTask re1Task("RE1");
RelayTask re2Task("RE2");
RelayTask re3Task("RE3");

// Debug print functions
void debugPrint(const String &msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

void debugPrintln(const String &msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

void turnOffAllRelays() {
  digitalWrite(RE1_PIN, HIGH);
  digitalWrite(RE2_PIN, HIGH);
  digitalWrite(RE3_PIN, HIGH);
  digitalWrite(RE4_PIN, HIGH);
  re1Task.active = false;
  re1Task.relayOn = false;
  re2Task.active = false;
  re2Task.relayOn = false;
  re3Task.active = false;
  re3Task.relayOn = false;
  debugPrintln("All relays turned OFF due to long switch hold.");
}

// Helper to start relay task
void startRelayTask(RelayTask &task, int relayPin, bool triggerRE4) {
  digitalWrite(relayPin, LOW);
  task.active = true;
  task.startTime = millis();
  task.re4Triggered = !triggerRE4; // If RE4 should be triggered, set false
  task.relayOn = true;
  debugPrint("Relay ON: ");
  debugPrintln(task.name);
}

// Helper to process relay task
void processRelayTask(RelayTask &task, int relayPin) {
  if (!task.active) return;
  unsigned long now = millis();

  // After FLAP_OPENING_TIME, trigger RE4 for 1s (if not already triggered)
  if (!task.re4Triggered && now - task.startTime >= FLAP_OPENING_TIME) {
    digitalWrite(RE4_PIN, LOW);
    delay(1000);
    digitalWrite(RE4_PIN, HIGH);
    task.re4Triggered = true;
    debugPrint("Relay RE4 triggered by: ");
    debugPrintln(task.name);
  }

  // After CENTRAL_FAN_RUN_TIME, turn off relay
  if (task.relayOn && now - task.startTime >= CENTRAL_FAN_RUN_TIME) {
    digitalWrite(relayPin, HIGH);
    task.active = false;
    task.relayOn = false;
    debugPrint("Relay OFF: ");
    debugPrintln(task.name);
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
  bme.begin(0x76); // Check your BME280 I2C address 0x76 or 0x77
}

void loop() {
  // Button handling
  static bool sw1Prev = HIGH, sw2Prev = HIGH;
  bool sw1 = digitalRead(SW1_PIN);
  bool sw2 = digitalRead(SW2_PIN);

  // SW1 pressed
  if (sw1Prev == HIGH && sw1 == LOW) {
    debugPrintln("Switch pressed: SW1");
    if (!re1Task.active) {
      startRelayTask(re1Task, RE1_PIN, true);
    } else {
      // If already active, just restart timer, don't trigger RE4
      startRelayTask(re1Task, RE1_PIN, false);
    }
  }
  // SW2 pressed
  if (sw2Prev == HIGH && sw2 == LOW) {
    debugPrintln("Switch pressed: SW2");
    if (!re2Task.active) {
      startRelayTask(re2Task, RE2_PIN, true);
    } else {
      startRelayTask(re2Task, RE2_PIN, false);
    }
  }
  // Switch hold detection
  if (sw1 == LOW) {
    if (sw1HoldStart == 0) sw1HoldStart = millis();
    if (millis() - sw1HoldStart > 3000) {
      turnOffAllRelays();
      // Wait until switch is released to avoid repeated triggers
      while (digitalRead(SW1_PIN) == LOW) delay(10);
      sw1HoldStart = 0;
      sw2HoldStart = 0;
      return; // Skip rest of loop to avoid race conditions
    }
  } else {
    sw1HoldStart = 0;
  }

  if (sw2 == LOW) {
    if (sw2HoldStart == 0) sw2HoldStart = millis();
    if (millis() - sw2HoldStart > 3000) {
      turnOffAllRelays();
      while (digitalRead(SW2_PIN) == LOW) delay(10);
      sw1HoldStart = 0;
      sw2HoldStart = 0;
      return;
    }
  } else {
    sw2HoldStart = 0;
  }

  sw1Prev = sw1;
  sw2Prev = sw2;

  // Humidity every HUMIDITY_READOUT_PERIOD
  if (millis() - lastHumidityRead > HUMIDITY_READOUT_PERIOD) {
    lastHumidityRead = millis();
    float humidity = bme.readHumidity();
    debugPrint("Humidity: "); debugPrintln(String(humidity));
    if (humidity > HUMIDITY_THRESHOLD) {
      if (!re1Task.active) {
        debugPrint("Humidity triggered relay: ");
        debugPrintln(re1Task.name);
        startRelayTask(re1Task, RE1_PIN, true);
      }
    }
  }

  // Current every CURRENT_REDOUT_PERIOD
  if (millis() - lastCurrentRead > CURRENT_READOUT_PERIOD) {
    lastCurrentRead = millis();
    int sensorValue = analogRead(ACS712_PIN);
    float voltage = sensorValue * (5.0 / 1023.0);
    float current = (voltage - 2.5) / 0.185; // ACS712 5A version
    current = abs(current);
    debugPrint("Current: "); debugPrintln(String(current * 1000)); // mA
    if (current * 1000 > CURRENT_THRESHOLD) {
      if (!re3Task.active) {
        debugPrint("Current triggered relay: ");
        debugPrintln(re3Task.name);
        startRelayTask(re3Task, RE3_PIN, true);
      }
    }
  }

  // Process relay tasks
  processRelayTask(re1Task, RE1_PIN);
  processRelayTask(re2Task, RE2_PIN);
  processRelayTask(re3Task, RE3_PIN);
  delay(50);
}