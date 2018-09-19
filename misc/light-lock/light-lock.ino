#include <Adafruit_NeoPixel.h>

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

const int NUM_PHOTORESISTORS = 2;
const int PIN_PHOTORESISTORS[NUM_PHOTORESISTORS] = {2, 3};

/**
   LED strips.
*/

const int DEFAULT_BRIGHTNESS = 100;

const uint16_t NEOPIXEL_NUMS[NUM_PHOTORESISTORS] = {
  1, 1
};

const uint8_t NEOPIXEL_PINS[NUM_PHOTORESISTORS] = {
  11, 12
};

const uint32_t COLORS[NUM_PHOTORESISTORS] = {
  Adafruit_NeoPixel::Color(255, 0, 255),
  Adafruit_NeoPixel::Color(255, 0, 255)
};

Adafruit_NeoPixel pixelStrips[NUM_PHOTORESISTORS];

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

bool arePhotoActivated() {
  for (int i = 0; i < NUM_PHOTORESISTORS; i++) {
    if (digitalRead(PIN_PHOTORESISTORS[i]) == HIGH) {
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
   LED functions.
*/

void initLedStrips() {
  for (int i = 0; i < NUM_PHOTORESISTORS; i++) {
    pixelStrips[i] = Adafruit_NeoPixel(NEOPIXEL_NUMS[i], NEOPIXEL_PINS[i], NEO_GRB + NEO_KHZ800);
    pixelStrips[i].begin();
    pixelStrips[i].setBrightness(DEFAULT_BRIGHTNESS);
    pixelStrips[i].clear();
    pixelStrips[i].show();
  }
}

void updateLedStrip(int idx) {
  pixelStrips[idx].clear();

  if (progState.isOpen) {
    for (int j = 0; j < NEOPIXEL_NUMS[idx]; j++) {
      pixelStrips[idx].setPixelColor(j, COLORS[idx]);
    }
  }

  pixelStrips[idx].show();
}

void updateLedStrips() {
  for (int i = 0; i < NUM_PHOTORESISTORS; i++) {
    updateLedStrip(i);
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
  initLedStrips();

  Serial.println(">> Starting light lock program");
}

void loop() {
  checkPhotoUpdateCounter();
  checkCounterToOpenRelay();
  updateLedStrips();
  delay(DELAY_LOOP_MS);
}
