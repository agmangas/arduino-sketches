#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

typedef struct potInfo {
  int potPin;
} PotInfo;

typedef struct programState {
  bool isEncoderActive;
  int encoderLevel;
  bool maxEncoderLevelReached;
} ProgramState;

/**
   Rotary encoder pins:
   A and B: white and green wires
   Red wire: 5V
   Black wire: GND
*/
const int ENC_PIN_A = 8;
const int ENC_PIN_B = 9;

const byte PIN_AUDIO_TRACK_POTS = 11;
const byte PIN_AUDIO_TRACK_ENCODER = 12;

const byte FINAL_RELAY_PIN = 7;

const uint16_t NEOPIXEL_NUM = 244;
const uint8_t NEOPIXEL_PIN = 5;
const int STRIP_BLOCK_LEN = 209;

const int ENCODER_BOUNCE_MS = 1000;
const int MAX_ENCODER_LEVEL = NEOPIXEL_NUM - STRIP_BLOCK_LEN;

const int ENC_RANGE_LO = 0;
const int ENC_RANGE_HI = 10;

const int POT_RANGE_LO = 1;
const int POT_RANGE_HI = 9;

const int TOTAL_POTS = 3;

const unsigned long POTS_SOLUTION_DELAY_MS = 3000;
const int POTS_SOLUTION_CHECKS = 10;

PotInfo potInfos[TOTAL_POTS] = {
  { .potPin = A0 },
  { .potPin = A1 },
  { .potPin = A2 }
};

/**
   5 Azul: Gatos
   2 Rojo: Arte
   7 Amarillo: Dormir
*/
int potSolutionKey[TOTAL_POTS] = { 5, 2, 5 };

Atm_analog pots[TOTAL_POTS];
Atm_controller potsController;
Atm_controller encoderController;
Atm_encoder rotEncoder;
Atm_timer encoderLevelTimer;

unsigned long rotCounter = 1;
const unsigned long ROT_COUNTER_DIVISOR = 8;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

const unsigned long POTS_STRIP_DELAY_STEP_MS = 100;

const uint32_t POTS_VALID_COLOR = pixelStrip.Color(80, 0, 255);
const uint32_t ENCODER_COLOR = pixelStrip.Color(80, 0, 255);
const uint32_t LIGHTNING_COLOR = pixelStrip.Color(255, 255, 255);

ProgramState programState = {
  .isEncoderActive = false,
  .encoderLevel = 0,
  .maxEncoderLevelReached = false
};

void setStripsOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void onPotChange(int idx, int v, int up) {
  Serial.print("onPotChange:: idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
  Serial.flush();
}

bool isPotsSolutionValid(int idx) {
  return isPotsSolutionValid();
}

bool isPotsSolutionValid() {
  for (int i = 0; i < TOTAL_POTS; i++) {
    if (pots[i].state() != potSolutionKey[i]) {
      return false;
    }
  }

  return true;
}

void activateValidPotsStrip() {
  for (int i = 0; i < STRIP_BLOCK_LEN; i++) {
    pixelStrip.setPixelColor(i, POTS_VALID_COLOR);
    pixelStrip.show();
    delay(POTS_STRIP_DELAY_STEP_MS);
  }
}

void onPotsSolutionValid(int idx, int v, int up) {
  if (programState.isEncoderActive) {
    return;
  }

  Serial.println("Valid pots: Verifying");

  unsigned long delayByCheck = POTS_SOLUTION_DELAY_MS / POTS_SOLUTION_CHECKS;

  for (int i = 0; i < POTS_SOLUTION_CHECKS; i++) {
    if (!isPotsSolutionValid()) {
      Serial.println("Pots changed: Verification failed");
      return;
    }

    delay(delayByCheck);
  }

  Serial.println("Pots stable: Verification OK");

  playTrack(PIN_AUDIO_TRACK_POTS);
  activateValidPotsStrip();
  programState.isEncoderActive = true;
}

void onMaxEncoderLevel(int idx, int v, int up) {
  if (programState.maxEncoderLevelReached) {
    return;
  }

  Serial.println("Max encoder level reached: Opening relay");

  stopHoldTrack(PIN_AUDIO_TRACK_ENCODER);
  programState.maxEncoderLevelReached = true;
  showLightningEffect();
  openRelay();
}

void onRotEncoderChange(int idx, int v, int up) {
  if (!programState.isEncoderActive) {
    return;
  }

  if (!programState.maxEncoderLevelReached) {
    playHoldTrack(PIN_AUDIO_TRACK_ENCODER);
  }

  Serial.print("onRotEncoderChange:: idx=");
  Serial.print(idx);
  Serial.print(" v=");
  Serial.print(v);
  Serial.println();
  Serial.flush();

  rotCounter++;

  if (rotCounter % ROT_COUNTER_DIVISOR == 0) {
    Serial.println("Encoder event");
    increaseEncoderLevel();
  }
}

void clearEncoderStripBlock() {
  for (int i = STRIP_BLOCK_LEN; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void showLightningEffect() {
  const int blockSize = 15;

  int limitIdx = STRIP_BLOCK_LEN;
  int pivotIdx = STRIP_BLOCK_LEN + MAX_ENCODER_LEVEL;

  while ((pivotIdx - blockSize) > limitIdx) {
    clearEncoderStripBlock();

    for (int i = pivotIdx; i > (pivotIdx - blockSize); i--) {
      pixelStrip.setPixelColor(i, LIGHTNING_COLOR);
    }

    pixelStrip.show();

    pivotIdx--;
  }

  clearEncoderStripBlock();
}

void updateEncoderStrip() {
  for (int i = STRIP_BLOCK_LEN; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  int activeLimit = STRIP_BLOCK_LEN + programState.encoderLevel;

  for (int i = STRIP_BLOCK_LEN; i < activeLimit; i++) {
    pixelStrip.setPixelColor(i, ENCODER_COLOR);
  }

  pixelStrip.show();
}

bool isMaxEncoderLevel() {
  return programState.encoderLevel >= MAX_ENCODER_LEVEL;
}

bool isMaxEncoderLevel(int idx) {
  return isMaxEncoderLevel();
}

void increaseEncoderLevel() {
  if (!isMaxEncoderLevel()) {
    programState.encoderLevel++;
    updateEncoderStrip();
    Serial.print("Encoder level: ");
    Serial.println(programState.encoderLevel);
  }
}

void decreaseEncoderLevel() {
  if (isMaxEncoderLevel()) {
    return;
  }

  if (programState.encoderLevel > 0) {
    programState.encoderLevel--;
    updateEncoderStrip();
    Serial.print("Encoder level: ");
    Serial.println(programState.encoderLevel);
  }
}

void onEncoderTimer(int idx, int v, int up) {
  if (programState.maxEncoderLevelReached) {
    showLightningEffect();
  } else {
    decreaseEncoderLevel();
  }
}

void initMachines() {
  for (int i = 0; i < TOTAL_POTS; i++) {
    pots[i]
    .begin(potInfos[i].potPin)
    .range(POT_RANGE_LO, POT_RANGE_HI)
    .onChange(onPotChange, i);
  }

  potsController
  .begin()
  .IF(isPotsSolutionValid)
  .onChange(true, onPotsSolutionValid);

  rotEncoder
  .begin(ENC_PIN_A, ENC_PIN_B)
  .range(ENC_RANGE_LO, ENC_RANGE_HI, true)
  .onChange(onRotEncoderChange);

  encoderLevelTimer
  .begin(ENCODER_BOUNCE_MS)
  .repeat(-1)
  .onTimer(onEncoderTimer)
  .start();

  encoderController
  .begin()
  .IF(isMaxEncoderLevel)
  .onChange(true, onMaxEncoderLevel);
}

void initStrip() {
  pixelStrip.begin();
  pixelStrip.setBrightness(200);
  pixelStrip.show();

  setStripsOff();
}

void lockRelay() {
  digitalWrite(FINAL_RELAY_PIN, LOW);
}

void openRelay() {
  digitalWrite(FINAL_RELAY_PIN, HIGH);
}

void initRelay() {
  pinMode(FINAL_RELAY_PIN, OUTPUT);
  lockRelay();
}

void playTrack(byte trackPin) {
  Serial.print("Playing track: ");
  Serial.println(trackPin);

  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

void playHoldTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
}

void stopHoldTrack(byte trackPin) {
  digitalWrite(trackPin, HIGH);
}

void initAudioPins() {
  pinMode(PIN_AUDIO_TRACK_POTS, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_POTS, HIGH);

  pinMode(PIN_AUDIO_TRACK_ENCODER, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_ENCODER, HIGH);
}

void setup() {
  initAudioPins();

  Serial.begin(9600);

  initMachines();
  initStrip();
  initRelay();

  Serial.println(">> Starting Caldera Neuronal Phase 2 program");
}

void loop() {
  automaton.run();
}


