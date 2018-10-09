#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
   Structs.
*/

typedef struct programState {
  bool isOpened;
} ProgramState;

ProgramState progState = {
  .isOpened = false
};

/**
   Digital switches constants.
*/

const int NUM_DIGITAL_SWITCH  = 3;

const int digitalSwitchPins[NUM_DIGITAL_SWITCH] = {
  7, 8, 9
};

Atm_button digitalSwitches[NUM_DIGITAL_SWITCH];

/**
   LED constants.
*/

const int DEFAULT_BRIGHTNESS = 120;

const uint16_t NEOPIXEL_NUMS[NUM_DIGITAL_SWITCH] = {
  1, 1, 1
};

const uint8_t NEOPIXEL_PINS[NUM_DIGITAL_SWITCH] = {
  4, 5, 6
};

Adafruit_NeoPixel pixelStrips[NUM_DIGITAL_SWITCH];

/**
   Relay
*/

const int PIN_RELAY = 3;

/**
   Digital switches functions.
*/

void onDigitalSwitch(int idx, int v, int up) {
  Serial.print("DigSwitch:");
  Serial.print(idx);
  Serial.print(":");
  Serial.println(v);
}

bool isDigitalSwitchOn(int idx) {
  return digitalSwitches[idx].state() == Atm_button::PRESSED;
}

bool allDigitalSwitchesOn() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    if (!isDigitalSwitchOn(i)) {
      return false;
    }
  }

  return true;
}

void initDigitalSwitches() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    digitalSwitches[i]
    .begin(digitalSwitchPins[i])
    .onPress(onDigitalSwitch, i);
  }
}

/**
   Relay functions
*/

void lockRelay() {
  digitalWrite(PIN_RELAY, LOW);
  progState.isOpened = false;
}

void openRelay() {
  digitalWrite(PIN_RELAY, HIGH);
  progState.isOpened = true;
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

void checkStatusToUpdateRelay() {
  if (allDigitalSwitchesOn() && !progState.isOpened) {
    Serial.println("All switches OK: Opening Relay");
    openRelay();
  } else if (!allDigitalSwitchesOn() && progState.isOpened) {
    Serial.println("Some switches are off: Locking Relay");
    lockRelay();
  }
}

/**
   LED functions.
*/

uint32_t getRandomReddishColor() {
  return Adafruit_NeoPixel::Color(
           random(200, 255),
           random(0, 70),
           0);
}

void initLedStrips() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    pixelStrips[i] = Adafruit_NeoPixel(NEOPIXEL_NUMS[i], NEOPIXEL_PINS[i], NEO_GRB + NEO_KHZ800);
    pixelStrips[i].begin();
    pixelStrips[i].setBrightness(DEFAULT_BRIGHTNESS);
    pixelStrips[i].clear();
    pixelStrips[i].show();
  }
}

void updateLedStrip(int idx) {
  pixelStrips[idx].clear();

  if (allDigitalSwitchesOn()) {
    for (int j = 0; j < NEOPIXEL_NUMS[idx]; j++) {
      pixelStrips[idx].setPixelColor(j, getRandomReddishColor());
    }
  }

  pixelStrips[idx].show();
}

void updateLedStrips() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    updateLedStrip(i);
  }
}

/**
   Entrypoint.
*/

void setup() {
  initDigitalSwitches();
  initLedStrips();
  initRelay();
}

void loop() {
  automaton.run();
  updateLedStrips();
  checkStatusToUpdateRelay();
}
