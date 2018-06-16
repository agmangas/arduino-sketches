#include <NeoEffects.h>
#include <NeoStrip.h>
#include <NeoWindow.h>
#include <Automaton.h>

const byte ID_EFFECT_NONE = 0;
const byte ID_EFFECT_STORM = 1;
const byte ID_EFFECT_CALM = 2;

typedef struct windowState {
  int counter;
  bool nextStepAllowed;
  byte effect;
} WindowState;

typedef struct joystickInfo {
  byte pinUp;
  byte pinDown;
  byte pinLeft;
  byte pinRight;
} JoystickInfo;

Atm_button joy01BtnUp;
Atm_button joy01BtnDown;
Atm_button joy01BtnLeft;
Atm_button joy01BtnRight;

Atm_button joy02BtnUp;
Atm_button joy02BtnDown;
Atm_button joy02BtnLeft;
Atm_button joy02BtnRight;

const int JOYSTICK_ID_01 = 1;
const int JOYSTICK_ID_02 = 2;

JoystickInfo joyInfo01 = {
  .pinUp = 4,
  .pinDown = 5,
  .pinLeft = 6,
  .pinRight = 7
};

JoystickInfo joyInfo02 = {
  .pinUp = 8,
  .pinDown = 9,
  .pinLeft = 10,
  .pinRight = 11
};

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

void onJoyUp(int idx, int v, int up) {
  Serial.print("U::");
  Serial.println(idx);
}

void onJoyDown(int idx, int v, int up) {
  Serial.print("D::");
  Serial.println(idx);
}

void onJoyLeft(int idx, int v, int up) {
  Serial.print("L::");
  Serial.println(idx);
}

void onJoyRight(int idx, int v, int up) {
  Serial.print("R::");
  Serial.println(idx);
}

void initJoysticks() {
  joy01BtnUp.begin(joyInfo01.pinUp)
  .onPress(onJoyUp, JOYSTICK_ID_01);

  joy01BtnDown.begin(joyInfo01.pinDown)
  .onPress(onJoyDown, JOYSTICK_ID_01);

  joy01BtnLeft.begin(joyInfo01.pinLeft)
  .onPress(onJoyLeft, JOYSTICK_ID_01);

  joy01BtnRight.begin(joyInfo01.pinRight)
  .onPress(onJoyRight, JOYSTICK_ID_01);

  joy02BtnUp.begin(joyInfo02.pinUp)
  .onPress(onJoyUp, JOYSTICK_ID_02);

  joy02BtnDown.begin(joyInfo02.pinDown)
  .onPress(onJoyDown, JOYSTICK_ID_02);

  joy02BtnLeft.begin(joyInfo02.pinLeft)
  .onPress(onJoyLeft, JOYSTICK_ID_02);

  joy02BtnRight.begin(joyInfo02.pinRight)
  .onPress(onJoyRight, JOYSTICK_ID_02);
}

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

  winState.nextStepAllowed = false;

  switch (winState.counter) {
    case 0:
      window.setFadeEfx(
        strip.Color(66, 63, 97),
        strip.Color(0, 71, 171),
        100, window.fadeTypeCycle, random(100, 200));
      break;
    default:
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

  winState.nextStepAllowed = false;

  switch (winState.counter) {
    case 0:
      window.setFadeEfx(
        strip.Color(0, 40, 171),
        strip.Color(0, 71, 171),
        0, window.fadeTypeJumpBack, random(0, 10));
      break;
    case 1:
      window.setBlinkEfx(
        strip.Color(0, 71, 171), 20, random(0, 30));
      break;
    case 2:
      window.setFadeEfx(
        strip.Color(125, 249, 255),
        strip.Color(0, 71, 171),
        0, window.fadeTypeJumpBack, random(0, 2));
      break;
    case 3:
      window.setMultiSparkleEfx(
        strip.Color(0, 71, 171),
        random(5, 20), random(5, 20),
        random(40, 60), random(10, 100));
      break;
    default:
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

  winState.effect == effect;
}

void setup() {
  randomSeed(analogRead(0));

  Serial.begin(9600);

  initJoysticks();
  initStrips();

  Serial.println(">> Starting Storm Catcher program");
}

void loop() {
  automaton.run();
  runStrips();
}
