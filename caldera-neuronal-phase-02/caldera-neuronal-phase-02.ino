#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#define SFX_TX 4
#define SFX_RX 3
#define SFX_RST 2

#define NEOPIXEL_NUM 60
#define NEOPIXEL_PIN 5

/**
   Rotary encoder pins:
   A and B are the white and green wires
   Red wire: 5V
   Black wire: GND
*/
#define ENC_PIN_A 6
#define ENC_PIN_B 7

typedef struct potInfo {
  int potPin;
} PotInfo;

typedef struct programState {
  bool isEncoderActive;
} ProgramState;

const int ENC_RANGE_LO = 0;
const int ENC_RANGE_HI = 50;

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

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

ProgramState programState = {
  .isEncoderActive = false
};

void setStripOff() {
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

void onPotsSolutionValid(int idx, int v, int up) {
  Serial.println("Potentiometers: Valid");
  programState.isEncoderActive = true;
}

void onRotEncoderChange(int idx, int v, int up) {
  Serial.print("onRotEncoderChange:: idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
  Serial.flush();
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
}

void initStrip() {
  pixelStrip.begin();
  pixelStrip.setBrightness(250);
  pixelStrip.show();

  setStripOff();
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
