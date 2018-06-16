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

const uint16_t NEOPIX_NUM_01 = 150;
const uint8_t NEOPIX_PIN_01 = 12;

const uint16_t NEOPIX_NUM_02 = 150;
const uint8_t NEOPIX_PIN_02 = 13;

NeoStrip strip01 = NeoStrip(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
NeoStrip strip02 = NeoStrip(NEOPIX_NUM_02, NEOPIX_PIN_02, NEO_GRB + NEO_KHZ800);

NeoWindow window01 = NeoWindow(&strip01, 40, NEOPIX_NUM_01);
NeoWindow window02 = NeoWindow(&strip02, 0, NEOPIX_NUM_02);

WindowState win01State = {
  .counter = 0,
  .nextStepAllowed = true,
  .effect = ID_EFFECT_NONE
};

WindowState win02State = {
  .counter = 0,
  .nextStepAllowed = true,
  .effect = ID_EFFECT_NONE
};

const int LEN_EFFECTS_QUEUE = 2;
int effectsCounter = 0;

byte effectsQueue[LEN_EFFECTS_QUEUE] = {
  ID_EFFECT_CALM,
  ID_EFFECT_STORM
};

void initStrip(NeoStrip &strip) {
  strip.begin();
  strip.setBrightness(255);
  strip.clearStrip();
  strip.show();
}

void initStrips() {
  initStrip(strip01);
  initStrip(strip02);
}

void resetWindow(WindowState &winState, NeoWindow &window) {
  winState.counter = 0;
  winState.nextStepAllowed = true;
  winState.effect = ID_EFFECT_NONE;
  window.setNoEfx();
}

void runCalmEffect(WindowState &winState, NeoWindow &window, NeoStrip &strip) {
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
        strip.Color(0, 40, 171),
        strip.Color(51, 65, 106),
        20, window.fadeTypeCycle, random(1, 5));
      break;
    default:
      Serial.println("Calm::Reset");
      resetWindow(winState, window);
  }
}

void runStormEffect(WindowState &winState, NeoWindow &window, NeoStrip &strip) {
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
        strip.Color(0, 40, 171),
        strip.Color(0, 71, 171),
        0, window.fadeTypeJumpBack, random(1, 10));
      break;
    case 1:
      Serial.println("Storm::1");
      window.setBlinkEfx(
        strip.Color(0, 71, 171), 20, random(1, 30));
      break;
    case 2:
      Serial.println("Storm::2");
      window.setFadeEfx(
        strip.Color(125, 249, 255),
        strip.Color(0, 71, 171),
        0, window.fadeTypeJumpBack, random(1, 2));
      break;
    case 3:
      Serial.println("Storm::3");
      window.setMultiSparkleEfx(
        strip.Color(230, 230, 230),
        random(5, 20), random(5, 20),
        random(40, 60), random(10, 100));
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
  runCalmEffect(win01State, window01, strip01);
  runStormEffect(win01State, window01, strip01);
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
  randomSeed(analogRead(0));

  Serial.begin(9600);

  initStrips();

  Serial.println(">> Starting Storm Catcher program");
}

void loop() {
  runStrips();
}
