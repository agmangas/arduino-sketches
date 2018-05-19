#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#define SFX_TX 4
#define SFX_RX 3
#define SFX_RST 2

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
  bool isEncoderDebounce;
  int encoderLevel;
} ProgramState;

const uint16_t NEOPIXEL_NUM = 30;
const uint8_t NEOPIXEL_PIN = 5;

const uint16_t NEOPIXEL_NUM_ENC = 30;
const uint8_t NEOPIXEL_PIN_ENC = 6;

const int ENCODER_TIMER_MS = 5000;
const int MAX_ENCODER_LEVEL = NEOPIXEL_NUM_ENC;

const int ENC_RANGE_LO = 0;
const int ENC_RANGE_HI = 30;

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
Atm_encoder rotEncoder;
Atm_timer encoderLevelTimer;

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStripEnc = Adafruit_NeoPixel(NEOPIXEL_NUM_ENC, NEOPIXEL_PIN_ENC, NEO_GRB + NEO_KHZ800);

const unsigned long POTS_STRIP_DELAY_STEP_MS = 100;

const uint32_t POTS_VALID_COLOR = pixelStripEnc.Color(200, 0, 0);
const uint32_t ENCODER_COLOR = pixelStripEnc.Color(200, 0, 0);

ProgramState programState = {
  .isEncoderActive = false,
  .isEncoderDebounce = false,
  .encoderLevel = 0
};

void setStripsOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  for (int i = 0; i < NEOPIXEL_NUM_ENC; i++) {
    pixelStripEnc.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
  pixelStripEnc.show();
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
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
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

  if (v == ENC_RANGE_LO && !programState.isEncoderDebounce) {
    Serial.println("Encoder event");
    programState.isEncoderDebounce = true;
    increaseEncoderLevel();
  } else if (v >= (mean - tolerance) && v <= (mean + tolerance)) {
    programState.isEncoderDebounce = false;
  }

  updateEncoderStrip();
}

void updateEncoderStrip() {
  for (int i = 0; i < NEOPIXEL_NUM_ENC; i++) {
    pixelStripEnc.setPixelColor(i, 0, 0, 0);
  }

  for (int i = 0; i < programState.encoderLevel; i++) {
    pixelStripEnc.setPixelColor(i, ENCODER_COLOR);
  }

  pixelStripEnc.show();
}

void increaseEncoderLevel() {
  if (programState.encoderLevel < MAX_ENCODER_LEVEL) {
    programState.encoderLevel++;
    Serial.print("Encoder level: ");
    Serial.println(programState.encoderLevel);
  }
}

void decreaseEncoderLevel(int idx, int v, int up) {
  if (programState.encoderLevel >= MAX_ENCODER_LEVEL) {
    return;
  }

  if (programState.encoderLevel > 0) {
    programState.encoderLevel--;
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
  .onTimer(decreaseEncoderLevel);
}

void initStrip() {
  pixelStrip.begin();
  pixelStrip.setBrightness(200);
  pixelStrip.show();

  pixelStripEnc.begin();
  pixelStripEnc.setBrightness(200);
  pixelStripEnc.show();

  setStripsOff();
}

void initSfx() {
  ss.begin(9600);
  sfx.reset();
}

void setup() {
  Serial.begin(9600);

  initMachines();
  initStrip();
  initSfx();

  Serial.println(">> Starting Caldera Neuronal Phase 2 program");
}

void loop() {
  automaton.run();
}
