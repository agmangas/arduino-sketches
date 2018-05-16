#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#define SFX_TX 4
#define SFX_RX 3
#define SFX_RST 2

#define NEOPIXEL_NUM 60
#define NEOPIXEL_PIN 5

typedef struct potInfo {
  int potPin;
} PotInfo;

const int POT_RANGE_LO = 0;
const int POT_RANGE_HI = 10;

const int TOTAL_POTS = 3;

PotInfo potInfos[TOTAL_POTS] = {
  { .potPin = A0 },
  { .potPin = A1 },
  { .potPin = A2 }
};

Atm_analog pots[TOTAL_POTS];

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setStripOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void onPotChange(int idx, int v, int up) {
  Serial.print("onPotChange idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
}

void initMachines() {
  for (int i = 0; i < TOTAL_POTS; i++) {
    pots[i]
    .begin(potInfos[i].potPin)
    .range(POT_RANGE_LO, POT_RANGE_HI)
    .onChange(onPotChange, i);
  }
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
