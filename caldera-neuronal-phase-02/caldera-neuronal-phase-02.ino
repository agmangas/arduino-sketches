#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
   Rotary encoder pins:
   A and B are the white and green wires
   Red wire: 5V
   Black wire: GND
*/
#define ENC_PIN_A 8
#define ENC_PIN_B 9

typedef struct potInfo {
  int potPin;
} PotInfo;

typedef struct programState {
  bool isEncoderActive;
  bool isEncoderInDebounce;
  int encoderLevel;
  bool maxEncoderLevelReached;
} ProgramState;

const uint16_t NEOPIXEL_NUM = 300;
const uint8_t NEOPIXEL_PIN = 5;
const int STRIP_BLOCK_LEN = 150;

const int ENCODER_TIMER_MS = 5000;
const int MAX_ENCODER_LEVEL = NEOPIXEL_NUM - STRIP_BLOCK_LEN;

const int ENC_RANGE_LO = 0;
const int ENC_RANGE_HI = 10;

const int POT_RANGE_LO = 0;
const int POT_RANGE_HI = 10;

const int TOTAL_POTS = 3;

PotInfo potInfos[TOTAL_POTS] = {
  { .potPin = A0 },
  { .potPin = A1 },
  { .potPin = A2 }
};

int potSolutionKey[TOTAL_POTS] = { 3, 4, 5 };

Atm_analog pots[TOTAL_POTS];
Atm_controller potsController;
Atm_controller encoderController;
Atm_encoder rotEncoder;
Atm_timer encoderLevelTimer;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

const unsigned long POTS_STRIP_DELAY_STEP_MS = 100;

const uint32_t POTS_VALID_COLOR = pixelStrip.Color(200, 0, 0);
const uint32_t ENCODER_COLOR = pixelStrip.Color(200, 0, 0);

ProgramState programState = {
  .isEncoderActive = false,
  .isEncoderInDebounce = false,
  .encoderLevel = 0,
  .maxEncoderLevelReached = false
};

void setStripsOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void onPotChange(int idx, int v, int up) {
  Serial.print("onPotChange:: idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
  Serial.flush();
}

bool isPotsSolutionValid(int idx) {
  for (int i = 0; i < TOTAL_POTS; i++) {
    if (pots[i].state() != potSolutionKey[i]) {
      return false;
    }
  }

  return true;
}

void activateValidPotsStrip() {
  for (int i = 0; i < STRIP_BLOCK_LEN; i++) {
    pixelStrip.setPixelColor(i, POTS_VALID_COLOR);
    pixelStrip.show();
    delay(POTS_STRIP_DELAY_STEP_MS);
  }
}

void onPotsSolutionValid(int idx, int v, int up) {
  if (programState.isEncoderActive) {
    return;
  }

  Serial.println("Potentiometers: Valid combination");

  activateValidPotsStrip();
  programState.isEncoderActive = true;
}

void onMaxEncoderLevel(int idx, int v, int up) {
  if (programState.maxEncoderLevelReached) {
    return;
  }

  Serial.println("Max encoder level reached");

  programState.maxEncoderLevelReached = true;
}

void onRotEncoderChange(int idx, int v, int up) {
  if (!programState.isEncoderActive) {
    return;
  }

  Serial.print("onRotEncoderChange:: idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
  Serial.flush();

  int mean = (ENC_RANGE_LO + ENC_RANGE_HI) / 2;
  int tolerance = ENC_RANGE_HI * 0.1;

  if (v == ENC_RANGE_LO && !programState.isEncoderInDebounce) {
    Serial.println("Encoder event");
    programState.isEncoderInDebounce = true;
    increaseEncoderLevel();
  } else if (v >= (mean - tolerance) && v <= (mean + tolerance)) {
    programState.isEncoderInDebounce = false;
  }
}

void updateEncoderStrip() {
  for (int i = STRIP_BLOCK_LEN; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  int activeLimit = STRIP_BLOCK_LEN + programState.encoderLevel;

  for (int i = STRIP_BLOCK_LEN; i < activeLimit; i++) {
    pixelStrip.setPixelColor(i, ENCODER_COLOR);
  }

  pixelStrip.show();
}

bool isMaxEncoderLevel() {
  return programState.encoderLevel >= MAX_ENCODER_LEVEL;
}

bool isMaxEncoderLevel(int idx) {
  return isMaxEncoderLevel();
}

void increaseEncoderLevel() {
  if (!isMaxEncoderLevel()) {
    programState.encoderLevel++;
    updateEncoderStrip();
    Serial.print("Encoder level: ");
    Serial.println(programState.encoderLevel);
  }
}

void decreaseEncoderLevel(int idx, int v, int up) {
  if (isMaxEncoderLevel()) {
    return;
  }

  if (programState.encoderLevel > 0) {
    programState.encoderLevel--;
    updateEncoderStrip();
    Serial.print("Encoder level: ");
    Serial.println(programState.encoderLevel);
  }
}

void initMachines() {
  for (int i = 0; i < TOTAL_POTS; i++) {
    pots[i]
    .begin(potInfos[i].potPin)
    .range(POT_RANGE_LO, POT_RANGE_HI)
    .onChange(onPotChange, i);
  }

  potsController
  .begin()
  .IF(isPotsSolutionValid)
  .onChange(true, onPotsSolutionValid);

  rotEncoder.begin(ENC_PIN_A, ENC_PIN_B)
  .range(ENC_RANGE_LO, ENC_RANGE_HI, true)
  .onChange(onRotEncoderChange);

  encoderLevelTimer
  .begin(ENCODER_TIMER_MS)
  .repeat(-1)
  .onTimer(decreaseEncoderLevel)
  .start();

  encoderController
  .begin()
  .IF(isMaxEncoderLevel)
  .onChange(true, onMaxEncoderLevel);
}

void initStrip() {
  pixelStrip.begin();
  pixelStrip.setBrightness(150);
  pixelStrip.show();

  setStripsOff();
}

void setup() {
  Serial.begin(9600);

  initMachines();
  initStrip();

  Serial.println(">> Starting Caldera Neuronal Phase 2 program");
}

void loop() {
  automaton.run();
}
