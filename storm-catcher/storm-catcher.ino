#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
  Structs
*/

typedef struct joystickInfo {
  byte pinUp;
  byte pinDown;
  byte pinLeft;
  byte pinRight;
} JoystickInfo;

typedef struct targetDots {
  byte* indexes;
  uint32_t* colors;
} TargetDots;

typedef struct playerDot {
  int idxStart;
  uint32_t color;
  bool isBlinking;
  bool isBlinkOn;
  Adafruit_NeoPixel &strip;
  byte matchCounter;
  TargetDots targets;
} PlayerDot;

/**
  Configuration and ID constants
*/

const int DOT_SIZE = 5;
const int DELAY_LOOP_MS = 5;
const int RANDOMIZE_TIMER_MS = 1500;

const int BUTTON_ID_01 = 100;
const int BUTTON_ID_02 = 200;

const int JOYSTICK_ID_01 = 1;
const int JOYSTICK_ID_02 = 2;

const int NUM_TARGETS = 4;

/**
   Color consts and arrays
*/

const uint32_t COLOR_PLAYER = Adafruit_NeoPixel::Color(255, 255, 255);
const uint32_t COLOR_TARGET_DONE = Adafruit_NeoPixel::Color(0, 0, 0);

uint32_t targetColors[NUM_TARGETS] = {
  Adafruit_NeoPixel::Color(255, 0, 0),
  Adafruit_NeoPixel::Color(0, 255, 0),
  Adafruit_NeoPixel::Color(0, 0, 255),
  Adafruit_NeoPixel::Color(255, 255, 0)
};

/**
   Initialization of joystick info structs
*/

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

/**
  Automator machines
*/

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

/**
   Relay pin
*/

const byte FINAL_RELAY_PIN = A0;

/**
   Audio track pins
*/

const byte PIN_AUDIO_TRACK_SUCCESS = A1;
const byte PIN_AUDIO_TRACK_FINAL = A2;

/**
   Button pins
*/

const byte BUTTON_P01_PIN = 2;
const byte BUTTON_P02_PIN = 3;

/**
   LED strips initialization
*/

const uint16_t NEOPIX_NUM_01 = 120;
const uint8_t NEOPIX_PIN_01 = 12;

const uint16_t NEOPIX_NUM_02 = 120;
const uint8_t NEOPIX_PIN_02 = 13;

Adafruit_NeoPixel strip01 = Adafruit_NeoPixel(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip02 = Adafruit_NeoPixel(NEOPIX_NUM_02, NEOPIX_PIN_02, NEO_GRB + NEO_KHZ800);

/**
   Initialization of PlayerDot structs
*/

byte targetsIdxP1[NUM_TARGETS] = {0, 0, 0, 0};

TargetDots targetsP1 = {
  .indexes = targetsIdxP1,
  .colors = targetColors
};

PlayerDot dotP1 = {
  .idxStart = 30,
  .color = COLOR_PLAYER,
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip01,
  .matchCounter = 0,
  .targets = targetsP1
};

byte targetsIdxP2[NUM_TARGETS] = {0, 0, 0, 0};

TargetDots targetsP2 = {
  .indexes = targetsIdxP2,
  .colors = targetColors
};

PlayerDot dotP2 = {
  .idxStart = 30,
  .color = COLOR_PLAYER,
  .isBlinking = true,
  .isBlinkOn = false,
  .strip = strip02,
  .matchCounter = 0,
  .targets = targetsP2
};

/**
   Program state variables
*/

bool relayOpened = false;

/**
   PlayerDot functions
*/

// Moving "right" = Moving away from pixel index 0

void movePlayerDotRight(PlayerDot &dot) {
  int idxTarget = dot.idxStart + DOT_SIZE;
  int limit = dot.strip.numPixels() - DOT_SIZE;

  if (idxTarget > limit) {
    idxTarget = limit;
  }

  dot.idxStart = idxTarget;
}

// Moving "left" = Moving towards pixel index 0

void movePlayerDotLeft(PlayerDot &dot) {
  int idxTarget = dot.idxStart - DOT_SIZE;

  if (idxTarget < 0) {
    idxTarget = 0;
  }

  dot.idxStart = idxTarget;
}

void drawPlayerDot(PlayerDot &dot) {
  int idxStart;

  for (int i = dot.matchCounter; i < NUM_TARGETS; i++) {
    idxStart = dot.targets.indexes[i];

    for (int j = idxStart; j < idxStart + DOT_SIZE; j++) {
      dot.strip.setPixelColor(j, dot.targets.colors[i]);
    }
  }

  uint32_t drawColor = dot.color;

  if (dot.isBlinking && dot.isBlinkOn) {
    dot.isBlinkOn = false;
    drawColor = Adafruit_NeoPixel::Color(0, 0, 0);
  } else if (dot.isBlinking && !dot.isBlinkOn) {
    dot.isBlinkOn = true;
  }

  for (int i = dot.idxStart; i < dot.idxStart + DOT_SIZE; i++) {
    dot.strip.setPixelColor(i, drawColor);
  }
}

void randomizeTargetDots(PlayerDot &dot) {
  int numRemainingTargets = NUM_TARGETS - dot.matchCounter;

  if (numRemainingTargets <= 0) {
    return;
  }

  int totalPositions = dot.strip.numPixels() / DOT_SIZE;
  int patchSize = totalPositions / numRemainingTargets;

  int targetOffset = random(0, numRemainingTargets);
  int targetIdx;
  int targetPosition;

  for (int i = 0; i < numRemainingTargets; i++) {
    targetIdx = ((i + targetOffset) % numRemainingTargets) + dot.matchCounter;
    targetPosition = random(0, patchSize) + (i * patchSize);

    dot.targets.indexes[targetIdx] = targetPosition * DOT_SIZE;
  }
}

bool isTargetMatch(PlayerDot &dot) {
  if (dot.matchCounter >= NUM_TARGETS) {
    return false;
  }

  uint32_t nextTargetColor = dot.targets.colors[dot.matchCounter];
  uint32_t currColor = COLOR_TARGET_DONE;

  for (int i = 0; i < NUM_TARGETS; i++) {
    if (dot.idxStart == dot.targets.indexes[i]) {
      currColor = dot.targets.colors[i];
    }
  }

  if (currColor == COLOR_TARGET_DONE ||
      currColor != nextTargetColor) {
    return false;
  }

  return true;
}

void drawDots() {
  if (dotP1.matchCounter < NUM_TARGETS) {
    drawPlayerDot(dotP1);
  }

  if (dotP2.matchCounter < NUM_TARGETS) {
    drawPlayerDot(dotP2);
  }
}

/**
   Joystick functions
*/

void onJoyUp(int idx, int v, int up) {
  Serial.print("U:");
  Serial.println(idx);
}

void onJoyDown(int idx, int v, int up) {
  Serial.print("D:");
  Serial.println(idx);
}

void onJoyLeft(int idx, int v, int up) {
  Serial.print("L:");
  Serial.println(idx);

  switch (idx) {
    case JOYSTICK_ID_01:
      movePlayerDotLeft(dotP1);
      break;
    case JOYSTICK_ID_02:
      movePlayerDotLeft(dotP2);
      break;
  }
}

void onJoyRight(int idx, int v, int up) {
  Serial.print("R:");
  Serial.println(idx);

  switch (idx) {
    case JOYSTICK_ID_01:
      movePlayerDotRight(dotP1);
      break;
    case JOYSTICK_ID_02:
      movePlayerDotRight(dotP2);
      break;
  }
}

void initJoysticks() {
  joy01BtnUp
  .begin(joyInfo01.pinUp)
  .onPress(onJoyUp, JOYSTICK_ID_01);

  joy01BtnDown
  .begin(joyInfo01.pinDown)
  .onPress(onJoyDown, JOYSTICK_ID_01);

  joy01BtnLeft
  .begin(joyInfo01.pinLeft)
  .onPress(onJoyLeft, JOYSTICK_ID_01);

  joy01BtnRight
  .begin(joyInfo01.pinRight)
  .onPress(onJoyRight, JOYSTICK_ID_01);

  joy02BtnUp
  .begin(joyInfo02.pinUp)
  .onPress(onJoyUp, JOYSTICK_ID_02);

  joy02BtnDown
  .begin(joyInfo02.pinDown)
  .onPress(onJoyDown, JOYSTICK_ID_02);

  joy02BtnLeft
  .begin(joyInfo02.pinLeft)
  .onPress(onJoyLeft, JOYSTICK_ID_02);

  joy02BtnRight
  .begin(joyInfo02.pinRight)
  .onPress(onJoyRight, JOYSTICK_ID_02);
}

/**
  Button and timer marchines functions
*/

void randomizeTargetDots() {
  randomizeTargetDots(dotP1);
  randomizeTargetDots(dotP2);
}

void onRandomizeTimer(int idx, int v, int up) {
  randomizeTargetDots();
}

void handleButtonPush(PlayerDot &dot) {
  if (isTargetMatch(dot)) {
    playTrack(PIN_AUDIO_TRACK_SUCCESS);
    dot.matchCounter++;
    showMatchSuccessEffect(dot.strip);
    randomizeTargetDots(dot);
    randomizeTimer.start();
  }
}

void onButtonChange(int idx, int v, int up) {
  Serial.print("B:");
  Serial.println(idx);

  switch (idx) {
    case BUTTON_ID_01:
      handleButtonPush(dotP1);
      break;
    case BUTTON_ID_02:
      handleButtonPush(dotP2);
      break;
  }

  if (dotP1.matchCounter >= NUM_TARGETS &&
      dotP2.matchCounter >= NUM_TARGETS) {
    openRelay();
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

/**
   LED strips functions
*/

void clearStrips() {
  strip01.clear();
  strip01.show();

  strip02.clear();
  strip02.show();
}

void showStrips() {
  strip01.show();
  strip02.show();
}

void initStrips() {
  strip01.begin();
  strip01.setBrightness(250);
  strip01.show();

  strip02.begin();
  strip02.setBrightness(250);
  strip02.show();

  clearStrips();
}

void showMatchSuccessEffect(Adafruit_NeoPixel &strip) {
  const int totalMs = 3200;
  const int blinkMs = 30;
  const int numIters = totalMs / (blinkMs * 2);

  for (int i = 0; i < numIters; i++) {
    for (int j = 0; j < strip.numPixels(); j++) {
      strip.setPixelColor(j, 0, 0, 0);
    }

    strip.show();
    delay(blinkMs);

    for (int j = 0; j < strip.numPixels(); j = j + 3) {
      strip.setPixelColor(j, 240, 240, 240);
    }

    strip.show();
    delay(blinkMs);
  }

  strip.clear();
  strip.show();
}

/**
   Relay functions
*/

void lockRelay() {
  digitalWrite(FINAL_RELAY_PIN, LOW);
}

void openRelay() {
  Serial.println("Relay:ON");
  digitalWrite(FINAL_RELAY_PIN, HIGH);

  if (!relayOpened) {
    playTrack(PIN_AUDIO_TRACK_FINAL);
    relayOpened = true;
  }
}

void initRelay() {
  pinMode(FINAL_RELAY_PIN, OUTPUT);
  lockRelay();
}

/**
   Audio FX board functions
*/

void playTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

void initAudioPins() {
  pinMode(PIN_AUDIO_TRACK_SUCCESS, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_SUCCESS, HIGH);

  pinMode(PIN_AUDIO_TRACK_FINAL, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_FINAL, HIGH);
}

/**
   Entrypoint
*/

void setup() {
  initAudioPins();

  Serial.begin(9600);

  initRelay();
  initJoysticks();
  initStrips();
  initMachines();
  randomizeTargetDots();

  Serial.println(">>");
}

void loop() {
  automaton.run();
  clearStrips();
  drawDots();
  showStrips();
  delay(DELAY_LOOP_MS);
}
