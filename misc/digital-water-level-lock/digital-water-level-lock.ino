/**
   Structs.
*/

typedef struct programState {
  bool isOpen;
  unsigned int counterWater;
  unsigned int counterEmpty;
} ProgramState;

ProgramState progState = {
  .isOpen = false,
  .counterWater = 0,
  .counterEmpty = 0
};

/**
   Misc constants.
*/

const int DELAY_LOOP_MS = 50;
const int MIN_ACTIVE_COUNTER = 20;

/**
   Water sensor constants.
*/

const int PIN_WATER_SENSOR = 5;

/**
   Relay constants.
*/

const int PIN_RELAY = 6;

/**
   Water sensor functions.
*/

void initWaterSensor() {
  pinMode(PIN_WATER_SENSOR, INPUT);
}

bool hasWater() {
  bool isWaterPresent = digitalRead(PIN_WATER_SENSOR) == LOW;
  Serial.print("Water:");
  Serial.println(isWaterPresent ? "YES" : "NO");
  return isWaterPresent;
}

void checkWaterSensorUpdateCounters() {
  if (hasWater()) {
    progState.counterEmpty = 0;
    progState.counterWater = progState.counterWater < MIN_ACTIVE_COUNTER ?
                             progState.counterWater + 1 : MIN_ACTIVE_COUNTER;
  } else {
    progState.counterWater = 0;
    progState.counterEmpty = progState.counterEmpty < MIN_ACTIVE_COUNTER ?
                             progState.counterEmpty + 1 : MIN_ACTIVE_COUNTER;
  }
}

/**
   Relay functions.
*/

void lockRelay() {
  Serial.println("Relay:Lock");
  digitalWrite(PIN_RELAY, LOW);
}

void openRelay() {
  Serial.println("Relay:Open");
  digitalWrite(PIN_RELAY, HIGH);
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

void checkCountersUpdateRelay() {
  if (progState.counterWater >= MIN_ACTIVE_COUNTER && !progState.isOpen) {
    digitalWrite(LED_BUILTIN, HIGH);
    openRelay();
    progState.isOpen = true;
  } else if (progState.counterEmpty >= MIN_ACTIVE_COUNTER && progState.isOpen) {
    digitalWrite(LED_BUILTIN, LOW);
    lockRelay();
    progState.isOpen = false;
  }
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initRelay();
  initWaterSensor();

  Serial.println(">> Starting digital water lock program");
}

void loop() {
  checkWaterSensorUpdateCounters();
  checkCountersUpdateRelay();
  delay(DELAY_LOOP_MS);
}
