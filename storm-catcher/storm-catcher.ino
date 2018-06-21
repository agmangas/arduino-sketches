#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

typedef struct joystickInfo {
  byte pinUp;
  byte pinDown;
  byte pinLeft;
  byte pinRight;
} JoystickInfo;

typedef struct stripDot {
  int idxStart;
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

StripDot dotP1 = {
  .idxStart = 0,
  .color = Adafruit_NeoPixel::Color(255, 255, 255),
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip01
};

StripDot targetP1 = {
  .idxStart = 0,
  .color = Adafruit_NeoPixel::Color(0, 255, 0),
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip01
};

StripDot dotP2 = {
  .idxStart = 0,
  .color = Adafruit_NeoPixel::Color(255, 255, 255),
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip02
};

StripDot targetP2 = {
  .idxStart = 0,
  .color = Adafruit_NeoPixel::Color(0, 255, 0),
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip02
};

/**
   Moving "right" = Moving away from pixel index 0
*/
void moveDotRight(StripDot &dot) {
  int idxTarget = dot.idxStart + DOT_SIZE;
  int limit = dot.strip.numPixels() - DOT_SIZE;

  if (idxTarget > limit) {
    idxTarget = limit;
  }

  dot.idxStart = idxTarget;
}

/**
   Moving "left" = Moving towards pixel index 0
*/
void moveDotLeft(StripDot &dot) {
  int idxTarget = dot.idxStart - DOT_SIZE;

  if (idxTarget < 0) {
    idxTarget = 0;
  }

  dot.idxStart = idxTarget;
}

void drawDot(StripDot &dot) {
  uint32_t drawColor = dot.color;

  if (dot.isBlinking && dot.isBlinkOn) {
    dot.isBlinkOn = false;
    drawColor = Adafruit_NeoPixel::Color(0, 0, 0);
  } else if (dot.isBlinking && !dot.isBlinkOn) {
    dot.isBlinkOn = true;
  }

  for (int i = 0; i < dot.idxStart + DOT_SIZE; i++) {
    dot.strip.setPixelColor(i, drawColor);
  }
}

bool isDotsMatch(StripDot &dot01, StripDot &dot02) {
  return dot01.idxStart == dot02.idxStart;
}

void randomizeDot(StripDot &dot) {
  int maxMultiplier = (dot.strip.numPixels() / DOT_SIZE) - 1;
  int multiplier = random(0, maxMultiplier);
  dot.idxStart = 0 + (DOT_SIZE * multiplier);
  dot.color = Adafruit_NeoPixel::Color(random(0, 255), random(0, 255), random(0, 255));
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
  randomizeDot(dotP1);
  randomizeDot(dotP2);
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
  drawDot(targetP1);
  drawDot(dotP1);

  drawDot(targetP2);
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
  strip01.setBrightness(200);
  strip01.show();

  strip02.begin();
  strip02.setBrightness(200);
  strip02.show();

  clearStrips();
}

void setup() {
  Serial.begin(9600);

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
