#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

typedef struct joystickInfo {
  byte pinUp;
  byte pinDown;
  byte pinLeft;
  byte pinRight;
} JoystickInfo;

typedef struct stripDot {
  int idxIni;
  uint32_t color;
  bool isBlinking;
  bool isBlinkOn;
  Adafruit_NeoPixel &strip;
} StripDot;

const int DOT_SIZE = 3;

Atm_button joy01BtnUp;
Atm_button joy01BtnDown;
Atm_button joy01BtnLeft;
Atm_button joy01BtnRight;

Atm_button joy02BtnUp;
Atm_button joy02BtnDown;
Atm_button joy02BtnLeft;
Atm_button joy02BtnRight;

Atm_timer randomizeTimer;

Atm_button buttonP01;
Atm_button buttonP02;

const int BUTTON_ID_01 = 100;
const int BUTTON_ID_02 = 200;

const int JOYSTICK_ID_01 = 1;
const int JOYSTICK_ID_02 = 2;

const byte BUTTON_P01_PIN = 2;
const byte BUTTON_P02_PIN = 3;

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

const int RANDOMIZE_TIMER_MS = 4000;

const uint16_t NEOPIX_NUM_01 = 150;
const uint8_t NEOPIX_PIN_01 = 12;

const uint16_t NEOPIX_NUM_02 = 150;
const uint8_t NEOPIX_PIN_02 = 13;

Adafruit_NeoPixel strip01 = Adafruit_NeoPixel(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip02 = Adafruit_NeoPixel(NEOPIX_NUM_02, NEOPIX_PIN_02, NEO_GRB + NEO_KHZ800);

const uint32_t COLOR_PLAYER = Adafruit_NeoPixel::Color(255, 255, 255);
const uint32_t COLOR_TARGET_CAUGHT = Adafruit_NeoPixel::Color(0, 0, 0);

/**
   The order in the following array is the same order
   that the player must follow to complete the game.
*/

const int NUM_TARGETS = 4;

uint32_t targetColors[NUM_TARGETS] = {
  Adafruit_NeoPixel::Color(255, 0, 0),
  Adafruit_NeoPixel::Color(0, 255, 0),
  Adafruit_NeoPixel::Color(0, 0, 255),
  Adafruit_NeoPixel::Color(255, 255, 0)
};

/**
   StripDot factory function.
*/
StripDot buildDot(uint32_t color, Adafruit_NeoPixel &strip) {
  StripDot theDot = {
    .idxIni = 0,
    .color = color,
    .isBlinking = true,
    .isBlinkOn = false,
    .strip = strip
  };

  return theDot;
}

/**
   Strip dots initialization.
*/

StripDot dotP1 = buildDot(COLOR_PLAYER, strip01);

StripDot targetsP1[NUM_TARGETS] = {
  buildDot(targetColors[0], strip01),
  buildDot(targetColors[1], strip01),
  buildDot(targetColors[2], strip01),
  buildDot(targetColors[3], strip01)
};

StripDot dotP2 = buildDot(COLOR_PLAYER, strip02);

StripDot targetsP2[NUM_TARGETS] = {
  buildDot(targetColors[0], strip02),
  buildDot(targetColors[1], strip02),
  buildDot(targetColors[2], strip02),
  buildDot(targetColors[3], strip02)
};

/**
   Moving "right" = Moving away from pixel index 0
*/
void moveDotRight(StripDot &dot) {
  int idxTarget = dot.idxIni + DOT_SIZE;
  int limit = dot.strip.numPixels() - DOT_SIZE;

  if (idxTarget > limit) {
    idxTarget = limit;
  }

  dot.idxIni = idxTarget;
}

/**
   Moving "left" = Moving towards pixel index 0
*/
void moveDotLeft(StripDot &dot) {
  int idxTarget = dot.idxIni - DOT_SIZE;

  if (idxTarget < 0) {
    idxTarget = 0;
  }

  dot.idxIni = idxTarget;
}

void drawDot(StripDot &dot) {
  uint32_t drawColor = dot.color;

  if (dot.isBlinking && dot.isBlinkOn) {
    dot.isBlinkOn = false;
    drawColor = Adafruit_NeoPixel::Color(0, 0, 0);
  } else if (dot.isBlinking && !dot.isBlinkOn) {
    dot.isBlinkOn = true;
  }

  for (int i = 0; i < dot.idxIni + DOT_SIZE; i++) {
    dot.strip.setPixelColor(i, drawColor);
  }
}

bool isDotsMatch(StripDot &dot01, StripDot &dot02) {
  return dot01.idxIni == dot02.idxIni;
}

void randomizeTargets(StripDot &playerDot, StripDot targets[]) {
  int blockSize = playerDot.strip.numPixels() / NUM_TARGETS;

  int blockIniIdx;
  int blockEndIdx;
  int targetIdx;

  for (int i = 0; i < NUM_TARGETS; i++) {
    blockIniIdx = 0 * blockSize;
    blockEndIdx = blockIniIdx + blockSize;
    targetIdx = randomTargetIdx(playerDot, blockIniIdx, blockEndIdx);
    targets[i].idxIni = targetIdx;
  }
}

int randomTargetIdx(StripDot &playerDot, int minIdx, int maxIdx) {
  int maxMultiplier = (playerDot.strip.numPixels() / DOT_SIZE) - 1;

  Serial.print("randomTargetIdx:: minIdx=");
  Serial.print(minIdx);
  Serial.print(" maxIdx=");
  Serial.print(maxIdx);
  Serial.print(" maxMultiplier=");
  Serial.print(maxMultiplier);
  Serial.println();
  Serial.flush();

  int currIniIdx;
  int currEndIdx;

  int optsCounter = 0;
  int firstOptIniIdx = -1;

  for (int i = 0; i <= maxMultiplier; i++) {
    currIniIdx = DOT_SIZE * i;
    currEndIdx = currIniIdx + DOT_SIZE;

    if (currIniIdx >= minIdx && currEndIdx <= maxIdx) {
      if (firstOptIniIdx == -1) {
        firstOptIniIdx = currIniIdx;
      }

      optsCounter++;
    }
  }

  Serial.print("randomTargetIdx:: optsCounter=");
  Serial.print(optsCounter);
  Serial.print(" firstOptIniIdx=");
  Serial.print(firstOptIniIdx);
  Serial.println();
  Serial.flush();

  if (optsCounter <= 0 || firstOptIniIdx == -1) {
    Serial.println("randomTargetIdx:: Not enough candidates");
    return -1;
  }

  int ret = firstOptIniIdx + random(0, optsCounter) * DOT_SIZE;

  const int maxAttempts = 20;
  int attemptsCounter = 0;

  while (playerDot.idxIni == ret) {
    ret = firstOptIniIdx + random(0, optsCounter) * DOT_SIZE;
    attemptsCounter++;

    if (attemptsCounter > maxAttempts) {
      Serial.println("randomTargetIdx:: Max randomization attempts reached");
      return -1;
    }
  }

  Serial.print("randomTargetIdx:: ret=");
  Serial.print(ret);
  Serial.println();
  Serial.flush();

  return ret;
}

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

  switch (idx) {
    case JOYSTICK_ID_01:
      moveDotLeft(dotP1);
      break;
    case JOYSTICK_ID_02:
      moveDotLeft(dotP2);
      break;
  }
}

void onJoyRight(int idx, int v, int up) {
  Serial.print("R::");
  Serial.println(idx);

  switch (idx) {
    case JOYSTICK_ID_01:
      moveDotRight(dotP1);
      break;
    case JOYSTICK_ID_02:
      moveDotRight(dotP2);
      break;
  }
}

void onRandomizeTimer(int idx, int v, int up) {
  randomizeTargets(dotP1, targetsP1);
  randomizeTargets(dotP2, targetsP2);
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

void onButtonChange(int idx, int v, int up) {
  Serial.print("Button::");
  Serial.println(idx);

  switch (idx) {
    case BUTTON_ID_01:
      break;
    case BUTTON_ID_02:
      break;
  }
}

void initMachines() {
  randomizeTimer
  .begin(RANDOMIZE_TIMER_MS)
  .repeat(-1)
  .onTimer(onRandomizeTimer)
  .start();

  buttonP01
  .begin(BUTTON_P01_PIN)
  .onPress(onButtonChange, BUTTON_ID_01);

  buttonP02
  .begin(BUTTON_P02_PIN)
  .onPress(onButtonChange, BUTTON_ID_02);
}

void drawDots() {
  drawDot(dotP1);
  drawDot(dotP2);
}

void clearStrip01() {
  strip01.clear();
  strip01.show();
}

void clearStrip02() {
  strip02.clear();
  strip02.show();
}

void clearStrips() {
  clearStrip01();
  clearStrip02();
}

void showStrips() {
  strip01.show();
  strip02.show();
}

void initStrips() {
  strip01.begin();
  strip01.setBrightness(255);
  strip01.show();

  strip02.begin();
  strip02.setBrightness(255);
  strip02.show();

  clearStrips();
}

void setup() {
  Serial.begin(9600);

  randomSeed(analogRead(0));

  initJoysticks();
  initStrips();
  initMachines();

  Serial.println(">> Starting Storm Catcher program");
}

void loop() {
  automaton.run();
  clearStrips();
  drawDots();
  showStrips();
}
