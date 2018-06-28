#include <NeoEffects.h>
#include <NeoStrip.h>
#include <NeoWindow.h>

const byte ID_EFFECT_NONE = 0;
const byte ID_EFFECT_STORM = 1;
const byte ID_EFFECT_CALM = 2;

typedef struct windowState {
  int counter;
  bool nextStepAllowed;
  byte effect;
} WindowState;

const uint16_t NEOPIX_NUM_01 = 300;
const uint8_t NEOPIX_PIN_01 = 12;

const uint32_t COLOR_ELECTRIC_BLUE = Adafruit_NeoPixel::Color(44, 117, 255);
const uint32_t COLOR_STORM_BLUE_LIGHT = Adafruit_NeoPixel::Color(66, 63, 97);
const uint32_t COLOR_STORM_BLUE_DARK = Adafruit_NeoPixel::Color(38, 37, 66);
const uint32_t COLOR_STORM_GREY = Adafruit_NeoPixel::Color(178, 209, 200);

NeoStrip strip01 = NeoStrip(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
NeoWindow window01 = NeoWindow(&strip01, 0, NEOPIX_NUM_01);

WindowState win01State = {
  .counter = 0,
  .nextStepAllowed = true,
  .effect = ID_EFFECT_NONE
};

const int LEN_EFFECTS_QUEUE = 5;

byte effectsQueue[LEN_EFFECTS_QUEUE] = {
  ID_EFFECT_CALM,
  ID_EFFECT_STORM,
  ID_EFFECT_CALM,
  ID_EFFECT_CALM,
  ID_EFFECT_STORM
};

int effectsCounter = 0;

void initStrip(NeoStrip &strip) {
  strip.begin();
  strip.setBrightness(255);
  strip.clearStrip();
  strip.show();
}

void initStrips() {
  initStrip(strip01);
}

void resetWindow(WindowState &winState, NeoWindow &window) {
  winState.counter = 0;
  winState.nextStepAllowed = true;
  winState.effect = ID_EFFECT_NONE;
  window.setNoEfx();
}

void runCalmEffect(WindowState &winState, NeoWindow &window) {
  if (winState.effect != ID_EFFECT_CALM) {
    return;
  }

  if (!winState.nextStepAllowed) {
    return;
  }

  Serial.println("Calm::Next");

  winState.nextStepAllowed = false;

  switch (winState.counter) {
    case 0:
      Serial.println("Calm::0");
      window.setFadeEfx(
        COLOR_STORM_BLUE_LIGHT,
        COLOR_STORM_GREY,
        20, window.fadeTypeCycle, random(2, 5));
      break;
    default:
      Serial.println("Calm::Reset");
      resetWindow(winState, window);
  }
}

void runStormEffect(WindowState &winState, NeoWindow &window) {
  if (winState.effect != ID_EFFECT_STORM) {
    return;
  }

  if (!winState.nextStepAllowed) {
    return;
  }

  Serial.println("Storm::Next");

  winState.nextStepAllowed = false;

  switch (winState.counter) {
    case 0:
      Serial.println("Storm::0");
      window.setFadeEfx(
        COLOR_STORM_BLUE_LIGHT,
        COLOR_STORM_GREY,
        0, window.fadeTypeJumpBack, random(1, 2));
      break;
    case 1:
      Serial.println("Storm::1");
      window.setBlinkEfx(
        COLOR_STORM_BLUE_DARK,
        20, random(1, 30));
      break;
    case 2:
      Serial.println("Storm::2");
      window.setFadeEfx(
        COLOR_STORM_BLUE_DARK,
        COLOR_STORM_BLUE_LIGHT,
        0, window.fadeTypeJumpBack, random(1, 2));
      break;
    case 3:
      Serial.println("Storm::3");
      window.setMultiSparkleEfx(
        COLOR_ELECTRIC_BLUE,
        random(5, 20), random(5, 20),
        random(20, 60), random(100, 150));
      break;
    default:
      Serial.println("Storm::Reset");
      resetWindow(winState, window);
  }
}

void updateAndShow(WindowState &winState, NeoWindow &window, NeoStrip &strip) {
  if (window.effectDone() && !winState.nextStepAllowed) {
    winState.counter++;
    winState.nextStepAllowed = true;
  }

  window.updateWindow();
  strip.show();
}

void runStrips() {
  NeoWindow::updateTime();

  setNextEffect(win01State);
  runCalmEffect(win01State, window01);
  runStormEffect(win01State, window01);
  updateAndShow(win01State, window01, strip01);
}

void setNextEffect(WindowState &winState) {
  if (winState.effect != ID_EFFECT_NONE) {
    return;
  }

  int effectIdx = effectsCounter % LEN_EFFECTS_QUEUE;
  byte effect = effectsQueue[effectIdx];
  effectsCounter++;

  Serial.print("Effect::");
  Serial.println(effect);

  winState.effect = effect;
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  initStrips();
  Serial.println(">> Starting Storm Effects program");
}

void loop() {
  runStrips();
}
