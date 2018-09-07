/**
   Structs.
*/

typedef struct programState {
  bool isOpen;
  int activeCounter;
} ProgramState;

ProgramState progState = {
  .isOpen = false,
  .activeCounter = 0
};

/**
   Misc constants.
*/

const int DELAY_LOOP_MS = 50;
const int MIN_ACTIVE_COUNTER = 20;

/**
   Photoresistor constants.
*/

const int NUM_PHOTORESISTORS = 1;
const int PIN_PHOTORESISTORS[NUM_PHOTORESISTORS] = {A0};
const int LIGHT_THRESHOLD = 50;

/**
   Relay constants.
*/

const int PIN_RELAY = 6;

/**
   Photoresistor functions.
*/

void initPhoto() {
  for (int i = 0; i < NUM_PHOTORESISTORS; i++) {
    pinMode(PIN_PHOTORESISTORS[i], INPUT);
  }
}

int readPhotoValue(int idx) {
  return analogRead(PIN_PHOTORESISTORS[idx]);
}

bool arePhotoActivated() {
  for (int i = 0; i < NUM_PHOTORESISTORS; i++) {
    if (readPhotoValue(i) > LIGHT_THRESHOLD) {
      return false;
    }
  }

  return true;
}

void checkPhotoUpdateCounter() {
  if (arePhotoActivated()) {
    progState.activeCounter++;
  } else {
    progState.activeCounter = 0;
  }
}

/**
   Relay functions.
*/

void lockRelay() {
  if (digitalRead(PIN_RELAY) == HIGH) {
    Serial.println("Relay:Lock");
  }

  digitalWrite(PIN_RELAY, LOW);
}

void openRelay() {
  if (digitalRead(PIN_RELAY) == LOW) {
    Serial.println("Relay:Open");
  }

  digitalWrite(PIN_RELAY, HIGH);
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

void checkCounterToOpenRelay() {
  if (progState.activeCounter >= MIN_ACTIVE_COUNTER && !progState.isOpen) {
    Serial.println("## Counter over threshold: Opening lock");
    progState.isOpen = true;
    digitalWrite(LED_BUILTIN, HIGH);
    openRelay();
  }
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initPhoto();
  initRelay();

  Serial.println(">> Starting light lock program");
}

void loop() {
  checkPhotoUpdateCounter();
  checkCounterToOpenRelay();
  delay(DELAY_LOOP_MS);
}
