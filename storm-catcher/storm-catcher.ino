#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
  Structs
*/

typedef struct programState {
  byte matchCounter;
  byte currColorIdx;
  byte targetPosition;
  bool relayOpened;
} ProgramState;

typedef struct joystickInfo {
  byte pinUp;
  byte pinDown;
  byte pinLeft;
  byte pinRight;
} JoystickInfo;

typedef struct playerDot {
  int idxStart;
  uint32_t color;
  bool isBlinking;
  bool isBlinkOn;
  int minIdxStart;
  int maxIdxStart;
} PlayerDot;

/**
  Configuration and ID constants
*/

const int DOT_SIZE = 5;
const int DELAY_LOOP_MS = 5;

const unsigned int RANDOMIZE_TIMER_MS_SLOW = 2000;
const unsigned int RANDOMIZE_TIMER_MS_FAST = 600;
const unsigned int RANDOMIZE_TIMER_MS_FASTER = 500;

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

const uint16_t NEOPIX_NUM_01 = 95;
const uint8_t NEOPIX_PIN_01 = 13;

const uint16_t NEOPIX_NUM_02 = 103;
const uint8_t NEOPIX_PIN_02 = 12;

const int PROGRESS_PATCH_SIZE = 40;

Adafruit_NeoPixel stripPlayers = Adafruit_NeoPixel(NEOPIX_NUM_01, NEOPIX_PIN_01, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripProgress = Adafruit_NeoPixel(NEOPIX_NUM_02, NEOPIX_PIN_02, NEO_GRB + NEO_KHZ800);

/**
   Initialization of dot structs and program state
*/

PlayerDot dotP2 = {
  .idxStart = 20,
  .color = COLOR_PLAYER,
  .isBlinking = true,
  .isBlinkOn = false,
  .minIdxStart = 0,
  .maxIdxStart = 9 * DOT_SIZE
};

PlayerDot dotP1 = {
  .idxStart = 75,
  .color = COLOR_PLAYER,
  .isBlinking = true,
  .isBlinkOn = false,
  .minIdxStart = dotP2.maxIdxStart + DOT_SIZE,
  .maxIdxStart = NEOPIX_NUM_01 - DOT_SIZE
};

ProgramState progState = {
  .matchCounter = 0,
  .currColorIdx = 0,
  .targetPosition = 0,
  .relayOpened = false
};

/**
   PlayerDot functions
*/

// Moving "right" = Moving away from pixel index 0

void movePlayerDotRight(PlayerDot &dot) {
  int idxTarget = dot.idxStart + DOT_SIZE;
  int limit = stripPlayers.numPixels() - DOT_SIZE;
  limit = limit > dot.maxIdxStart ? dot.maxIdxStart : limit;

  if (idxTarget > limit) {
    idxTarget = limit;
  }

  dot.idxStart = idxTarget;
}

// Moving "left" = Moving towards pixel index 0

void movePlayerDotLeft(PlayerDot &dot) {
  int idxTarget = dot.idxStart - DOT_SIZE;
  int limit = 0;
  limit = dot.minIdxStart > limit ? dot.minIdxStart : limit;

  Serial.println(limit);

  if (idxTarget < limit) {
    idxTarget = limit;
  }

  dot.idxStart = idxTarget;
}

void drawTargetDot() {
  uint32_t color = targetColors[progState.currColorIdx];
  int pos = progState.targetPosition;

  for (int i = pos; i < pos + DOT_SIZE; i++) {
    stripPlayers.setPixelColor(i, color);
  }
}

void drawPlayerDot(PlayerDot &dot) {
  uint32_t drawColor = dot.color;

  if (dot.isBlinking && dot.isBlinkOn) {
    dot.isBlinkOn = false;
    drawColor = Adafruit_NeoPixel::Color(0, 0, 0);
  } else if (dot.isBlinking && !dot.isBlinkOn) {
    dot.isBlinkOn = true;
  }

  for (int i = dot.idxStart; i < dot.idxStart + DOT_SIZE; i++) {
    stripPlayers.setPixelColor(i, drawColor);
  }
}

void randomizeTargetDot() {
  if (progState.matchCounter >= NUM_TARGETS) {
    return;
  }

  int totalPositions = stripPlayers.numPixels() / DOT_SIZE;
  int targetPosition = random(0, totalPositions) * DOT_SIZE;

  int currColorIdx = progState.matchCounter > 0 ?
                     random(progState.matchCounter, NUM_TARGETS) : 0;

  progState.currColorIdx = currColorIdx;
  progState.targetPosition = targetPosition;
}

bool isTargetMatch(PlayerDot &dot) {
  if (progState.matchCounter >= NUM_TARGETS) {
    return false;
  }

  bool isPositionMatch = dot.idxStart == progState.targetPosition;
  bool isColorMatch = progState.currColorIdx == progState.matchCounter;

  if (isPositionMatch == true && isColorMatch == false) {
    showColorEffect(0);
    return false;
  } else if (isPositionMatch == false || isColorMatch == false) {
    return false;
  }

  return true;
}

void drawDots() {
  if (progState.matchCounter >= NUM_TARGETS) {
    return;
  }

  drawTargetDot();
  drawPlayerDot(dotP1);
  drawPlayerDot(dotP2);
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

unsigned int getTimerIntervalMs() {
  switch (progState.matchCounter) {
    case 0:
      return RANDOMIZE_TIMER_MS_SLOW;
      break;
    case 1:
      return RANDOMIZE_TIMER_MS_FAST;
      break;
    default:
      return RANDOMIZE_TIMER_MS_FASTER;
      break;
  }
}

void onRandomizeTimer(int idx, int v, int up) {
  randomizeTargetDot();
}

void handleButtonPush(PlayerDot &dot) {
  if (!isTargetMatch(dot)) {
    return;
  }

  showColorEffect(targetColors[progState.matchCounter]);
  progState.matchCounter++;

  if (!enoughTargetsCaptured()) {
    playTrack(PIN_AUDIO_TRACK_SUCCESS);
  } else {
    playTrack(PIN_AUDIO_TRACK_FINAL);
  }

  updateShowProgressStrip();
  randomizeTargetDot();

  randomizeTimer
  .interval(getTimerIntervalMs())
  .start();
}

bool enoughTargetsCaptured() {
  return progState.matchCounter >= NUM_TARGETS;
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

  if (enoughTargetsCaptured()) {
    openRelay();
  }
}

void initMachines() {
  randomizeTimer
  .begin(getTimerIntervalMs())
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

void clearPlayerStrips() {
  stripPlayers.clear();
  stripPlayers.show();
}

void showPlayerStrips() {
  stripPlayers.show();
}

void initStrips() {
  stripPlayers.begin();
  stripPlayers.setBrightness(250);
  stripPlayers.show();

  stripProgress.begin();
  stripProgress.setBrightness(250);
  stripProgress.show();

  stripPlayers.clear();
  stripPlayers.show();

  stripProgress.clear();
  stripProgress.show();
}

uint32_t randomColor() {
  byte randVal = random(0, 3);

  if (randVal == 0) {
    return Adafruit_NeoPixel::Color(random(50, 250), 0, 0);
  } else if (randVal == 1) {
    return Adafruit_NeoPixel::Color(0, random(50, 250), 0);
  } else {
    return Adafruit_NeoPixel::Color(0, 0, random(50, 250));
  }
}

void showColorEffect(uint32_t color) {
  stripPlayers.clear();
  stripPlayers.show();

  bool isError = color == 0;

  int blinkMs;
  int blinkLen;
  uint32_t showColor;

  if (!isError) {
    blinkMs = 200;
    blinkLen = 4;
  } else {
    blinkMs = 10;
    blinkLen = 50;
  }

  for (int k = 0; k < blinkLen; k++) {
    if (!isError) {
      showColor = color;
    } else {
      showColor = randomColor();
    }

    for (int i = 0; i < stripPlayers.numPixels(); i++) {
      stripPlayers.setPixelColor(i, showColor);
    }

    stripPlayers.show();
    delay(blinkMs);

    for (int i = 0; i < stripPlayers.numPixels(); i++) {
      stripPlayers.setPixelColor(i, 0);
    }

    stripPlayers.show();
    delay(blinkMs);
  }

  stripPlayers.clear();
  stripPlayers.show();
}

void updateShowProgressStrip() {
  stripProgress.clear();
  stripProgress.show();

  int stepMs = 15;
  uint32_t color = Adafruit_NeoPixel::Color(255, 255, 255);
  int totalPixels = stripProgress.numPixels();

  for (int i = 0; i < totalPixels; i++) {
    stripProgress.setPixelColor(i, color);
    stripProgress.show();
    delay(stepMs);
  }

  stripProgress.clear();
  stripProgress.show();

  float filledRatio = ((float) progState.matchCounter) / NUM_TARGETS;
  filledRatio = (filledRatio > 1.0) ? 1.0 : filledRatio;
  int filledNum = floor(PROGRESS_PATCH_SIZE * filledRatio);

  for (int i = totalPixels; i >= (totalPixels - filledNum); i--) {
    stripProgress.setPixelColor(i, color);
    stripProgress.show();
    delay(stepMs);
  }
}

/**
   Relay functions
*/

void lockRelay() {
  digitalWrite(FINAL_RELAY_PIN, LOW);
}

void openRelay() {
  digitalWrite(FINAL_RELAY_PIN, HIGH);

  if (progState.relayOpened == false) {
    Serial.println("Relay:ON");
    progState.relayOpened = true;
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
  randomizeTargetDot();

  Serial.println(">>");
}

void loop() {
  automaton.run();
  clearPlayerStrips();
  drawDots();
  showPlayerStrips();
  delay(DELAY_LOOP_MS);
}
