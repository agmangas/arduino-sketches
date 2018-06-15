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

void initStrips() {
  strip01.begin();
  strip01.setBrightness(255);
  strip01.clearStrip();
  strip01.show();

  strip02.begin();
  strip02.setBrightness(255);
  strip02.clearStrip();
  strip02.show();
}

void resetWindow(WindowState &winState, NeoWindow &window) {
  winState.counter = 0;
  winState.nextStepAllowed = true;
  winState.effect = ID_EFFECT_NONE;
  window.setNoEfx();
}

void runStormEffect(WindowState &winState, NeoWindow &window, NeoStrip &strip) {
  if (winState.effect != ID_EFFECT_STORM) {
    return;
  }

  const int steps = 5;

  if (winState.counter % steps == 0 &&
      winState.nextStepAllowed) {
    Serial.println("W01::Step 1");
    winState.nextStepAllowed = false;

    window.setFadeEfx(
      strip.Color(0, 40, 171),
      strip.Color(0, 71, 171),
      0, window.fadeTypeJumpBack, random(2, 10));
  }

  if (winState.counter % steps == 1 &&
      winState.nextStepAllowed) {
    Serial.println("W01::Step 2");
    winState.nextStepAllowed = false;

    window.setBlinkEfx(
      strip.Color(0, 71, 171), 20, random(10, 30));
  }

  if (winState.counter % steps == 2 &&
      winState.nextStepAllowed) {
    Serial.println("W01::Step 3");
    winState.nextStepAllowed = false;

    window.setFadeEfx(
      strip.Color(125, 249, 255),
      strip.Color(0, 71, 171),
      0, window.fadeTypeJumpBack, random(1, 2));
  }

  if (winState.counter % steps == 3 &&
      winState.nextStepAllowed) {
    Serial.println("W01::Step 4");
    winState.nextStepAllowed = false;

    window.setMultiSparkleEfx(
      strip.Color(0, 71, 171),
      random(5, 20), random(5, 20),
      random(40, 60), random(10, 100));
  }

  if (winState.counter % steps == 4 &&
      winState.nextStepAllowed) {
    Serial.println("W01::Step 5");
    resetWindow(winState, window);
  }
}

void runStrips() {
  NeoWindow::updateTime();

  if (window01.effectDone() && !win01State.nextStepAllowed) {
    Serial.println("W01::Effect Done");
    win01State.counter++;
    win01State.nextStepAllowed = true;
  }

  window01.updateWindow();
  strip01.show();

  window02.updateWindow();
  strip02.show();
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
  win01State.effect = ID_EFFECT_STORM;
  runStormEffect(win01State, window01, strip01);
  runStrips();
}
