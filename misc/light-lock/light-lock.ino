#include <Adafruit_NeoPixel.h>

/**
   Structs.
*/

typedef struct programState {
  bool isOpen;
  int activeCounter;
  unsigned long openTimeMillis;
} ProgramState;

ProgramState progState = {
  .isOpen = false,
  .activeCounter = 0,
  .openTimeMillis = 0
};

/**
   Misc constants.
*/

const int DELAY_LOOP_MS = 50;
const int MIN_ACTIVE_COUNTER = 20;
const unsigned long OPEN_INTERVAL_MS = 30000;

/**
   Photoresistor constants.
*/

const int NUM_LDR = 2;
const int PIN_LDR[NUM_LDR] = {2, 3};

/**
   LED strips.
*/

const int DEFAULT_BRIGHTNESS = 100;

const uint16_t NEOPIXEL_NUMS[NUM_LDR] = {
  1, 1
};

const uint8_t NEOPIXEL_PINS[NUM_LDR] = {
  11, 12
};

const uint32_t COLORS[NUM_LDR] = {
  Adafruit_NeoPixel::Color(255, 0, 255),
  Adafruit_NeoPixel::Color(255, 0, 255)
};

Adafruit_NeoPixel pixelStrips[NUM_LDR];

/**
   Relay constants.
*/

const int PIN_RELAY = 6;

/**
   Photoresistor functions.
*/

void initPhoto() {
  for (int i = 0; i < NUM_LDR; i++) {
    pinMode(PIN_LDR[i], INPUT);
  }
}

bool arePhotoActivated() {
  for (int i = 0; i < NUM_LDR; i++) {
    if (digitalRead(PIN_LDR[i]) == HIGH) {
      return false;
    }
  }

  return true;
}

void checkPhotoUpdateCounter() {
  if (arePhotoActivated()) {
    progState.activeCounter = progState.activeCounter < MIN_ACTIVE_COUNTER ?
                              progState.activeCounter + 1 : MIN_ACTIVE_COUNTER;
  } else {
    progState.activeCounter = 0;
  }
}

/**
   LED functions.
*/

void initLedStrips() {
  for (int i = 0; i < NUM_LDR; i++) {
    pixelStrips[i] = Adafruit_NeoPixel(NEOPIXEL_NUMS[i], NEOPIXEL_PINS[i], NEO_GRB + NEO_KHZ800);
    pixelStrips[i].begin();
    pixelStrips[i].setBrightness(DEFAULT_BRIGHTNESS);
    pixelStrips[i].clear();
    pixelStrips[i].show();
  }
}

void updateLedStrips() {
  for (int i = 0; i < NUM_LDR; i++) {
    pixelStrips[i].clear();

    if (progState.isOpen) {
      for (int j = 0; j < NEOPIXEL_NUMS[i]; j++) {
        pixelStrips[i].setPixelColor(j, COLORS[i]);
      }
    }

    pixelStrips[i].show();
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

void checkCounterToOpenRelay() {
  if (progState.activeCounter >= MIN_ACTIVE_COUNTER && !progState.isOpen) {
    Serial.println("## Counter over threshold: Opening lock");
    progState.isOpen = true;
    progState.openTimeMillis = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    openRelay();
  }
}

bool shouldResetRelay() {
  if (!progState.isOpen) {
    return false;
  }

  unsigned long now = millis();

  bool isUndefined = progState.openTimeMillis == 0;
  bool isOverflow = now < progState.openTimeMillis;

  if (isUndefined || isOverflow) {
    return true;
  }

  return (now - progState.openTimeMillis) >= OPEN_INTERVAL_MS;
}

void resetRelay() {
  Serial.println("## Resetting relay");
  progState.isOpen = false;
  progState.openTimeMillis = 0;
  progState.activeCounter = 0;
  digitalWrite(LED_BUILTIN, LOW);
  lockRelay();
}

void resetRelayIfTimePassed() {
  if (shouldResetRelay()) {
    resetRelay();
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
  initLedStrips();

  Serial.println(">> Starting light lock program");
}

void loop() {
  checkPhotoUpdateCounter();
  checkCounterToOpenRelay();
  resetRelayIfTimePassed();
  updateLedStrips();
  delay(DELAY_LOOP_MS);
}
