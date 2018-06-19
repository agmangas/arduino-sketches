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

Adafruit_NeoPixel strip01 = Adafruit_NeoPixel(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip02 = Adafruit_NeoPixel(NEOPIX_NUM_02, NEOPIX_PIN_02, NEO_GRB + NEO_KHZ800);

/**
   Moving "right" = Moving away from pixel index 0
*/
void moveDotRight(StripDot &dot) {
}

/**
   Moving "left" = Moving towards pixel index 0
*/
void moveDotLeft(StripDot &dot) {
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

void clearStrip01() {
  strip01.clear();
  strip01.show();
}

void clearStrip02() {
  strip02.clear();
  strip02.show();
}

void initStrips() {
  strip01.begin();
  strip01.setBrightness(200);
  strip01.show();

  clearStrip01();

  strip02.begin();
  strip02.setBrightness(200);
  strip02.show();

  clearStrip02();
}

void setup() {
  Serial.begin(9600);

  initJoysticks();
  initStrips();

  Serial.println(">> Starting Storm Catcher program");
}

void loop() {
  automaton.run();
}
