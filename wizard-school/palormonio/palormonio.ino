#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>
#include "limits.h"

/**
  Program state.
*/

typedef struct programState {
  bool isAudioPatternOk;
  bool hasPlayedFinalAudio;
  unsigned long lastKnock;
  int nextLedSequenceStep;
  bool isRelayOpen;
  bool isLedSequenceRunning;
} ProgramState;

ProgramState progState = {
  .isAudioPatternOk = false,
  .hasPlayedFinalAudio = false,
  .lastKnock = 0,
  .nextLedSequenceStep = -1,
  .isRelayOpen = false,
  .isLedSequenceRunning = false
};

Atm_timer timerState;

const int STATE_TIMER_MS = 300;

/**
   Relay.
*/

const byte RELAY_PIN = 12;

/**
   Piezo knock sensor.
*/

const byte KNOCK_PIN = A0;
const byte KNOCK_THRESHOLD = 10;
const byte KNOCK_SAMPLERATE = 50;
const byte KNOCK_BUF_SIZE = 30;
const byte KNOCK_TRAINING_SIZE = 5;
const byte KNOCK_PATTERN_SIZE = 10;
const float KNOCK_TOLERANCE = 0.9;
const unsigned int KNOCK_DELAY_MS = 150;

unsigned int knockTrainingSet[KNOCK_TRAINING_SIZE][KNOCK_PATTERN_SIZE] = {
  {0, 478, 1279, 1609, 2292, 3481, 4069, 4961, 5282, 5808},
  {0, 595, 1448, 1775, 2354, 3607, 4188, 5086, 5404, 5944},
  {0, 596, 1446, 1751, 2409, 3688, 4271, 5180, 5485, 5994},
  {0, 621, 1549, 1840, 2462, 3616, 4211, 5093, 5391, 5950},
  {0, 556, 1542, 1846, 2477, 3397, 4462, 5206, 5460, 5992}
};

Atm_analog knockAnalog;
Atm_controller knockController;
CircularBuffer<unsigned long, KNOCK_BUF_SIZE> knockHistory;
float knockPattern[KNOCK_PATTERN_SIZE];
float meanKnockPatternDiff;

/**
   Microphones.
*/

const int MICROS_NUM = 5;
const int MICROS_AVG_BUF_SIZE = 5;
const int MICROS_SAMPLE_RATE_MS = 30;
const int MICROS_RANGE_MIN = 1;
const int MICROS_RANGE_MAX = 100;
const int MICROS_THRESHOLD = 10;

const byte MICRO_PINS[MICROS_NUM] = {
  A0, A1, A2, A3, A4
};

uint16_t MICROS_AVG_BUFS[MICROS_NUM][MICROS_AVG_BUF_SIZE];

Atm_analog microphones[MICROS_NUM];

/**
   Solution key.
*/

const int SOLUTION_SIZE = 10;

CircularBuffer<byte, SOLUTION_SIZE> triggerHistory;

const byte SOLUTION_KEY[SOLUTION_SIZE] = {
  0, 1, 2, 3, 4, 0, 1, 2, 3, 4
};

/**
   Audio FX.
*/

const byte PIN_AUDIO_RST = 9;
const byte PIN_AUDIO_ACT = 3;

const byte AUDIO_PINS[MICROS_NUM] = {
  8, 7, 6, 5, 4
};

const byte PIN_AUDIO_FINAL = 10;

/**
   LED strips.
*/

const int LED_BRIGHTNESS = 170;
const int LED_PIN = 11;
const int LED_NUM = 30;
const uint32_t COLOR_SEQUENCE = Adafruit_NeoPixel::Color(0, 0, 255);
const int SEQ_CLEAR_MS = 150;
const int LED_SEQ_DELAY_START = 2000;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

Atm_timer timerStartSequence;
Atm_timer timerLedSequence;
Atm_timer timerLedClear;

/**
   Knock sensor functions.
*/

void setKnockPattern() {
  for (int i = 0; i < KNOCK_PATTERN_SIZE; i++) {
    float val = 0;

    for (int j = 0; j < KNOCK_TRAINING_SIZE; j++) {
      val = val + knockTrainingSet[j][i];
    }

    val = val / KNOCK_TRAINING_SIZE;
    knockPattern[i] = val;
  }

  Serial.print(F("Knock pattern: "));

  for (int i = 0; i < KNOCK_PATTERN_SIZE; i++) {
    Serial.print(knockPattern[i]);
    Serial.print(F(", "));
  }

  Serial.println(F(""));
}

void setMeanKnockPatternDiff() {
  float val = 0;

  for (int i = 1; i < KNOCK_PATTERN_SIZE; i++) {
    val = val + (knockPattern[i] - knockPattern[i - 1]);
  }

  val = val / (KNOCK_PATTERN_SIZE - 1);

  Serial.print(F("Avg. knock pattern wait (ms): "));
  Serial.println(val);

  meanKnockPatternDiff = val;
}

bool isValidKnockPattern() {
  if (knockHistory.size() < KNOCK_PATTERN_SIZE) {
    return false;
  }

  float tol = meanKnockPatternDiff * KNOCK_TOLERANCE;

  int currIdx;
  unsigned long baseTime;
  unsigned long currTime;
  float timeMin;
  float timeMax;
  bool isValid;

  for (int i = 0; i <= knockHistory.size() - KNOCK_PATTERN_SIZE; i++) {
    isValid = true;
    baseTime = knockHistory[i];

    for (int j = 0; j < KNOCK_PATTERN_SIZE; j++) {
      currIdx = i + j;
      currTime = knockHistory[currIdx] - baseTime;
      timeMin = knockPattern[j] > tol ? knockPattern[j] - tol : 0;
      timeMax = knockPattern[j] + tol;

      if (currTime < timeMin || currTime > timeMax) {
        isValid = false;
        break;
      }
    }

    if (isValid) {
      return true;
    }
  }

  return false;
}

void onKnock(int idx, int v, int up) {
  if (!progState.isAudioPatternOk) {
    return;
  }

  unsigned long now = millis();

  if (now < progState.lastKnock) {
    Serial.println(F("Millis overflow: History reset"));
    progState.lastKnock = 0;
    knockHistory.clear();
    return;
  }

  unsigned long diff = now - progState.lastKnock;

  if (progState.lastKnock == 0 || diff >= KNOCK_DELAY_MS) {
    Serial.print(now);
    Serial.println(F("::Knock"));
    progState.lastKnock = now;
    knockHistory.push(now);
  }
}

void initKnockSensor() {
  setKnockPattern();
  setMeanKnockPatternDiff();

  knockAnalog
  .begin(KNOCK_PIN, KNOCK_SAMPLERATE);

  knockController
  .begin()
  .IF(knockAnalog, '>', KNOCK_THRESHOLD)
  .onChange(true, onKnock);
}

/**
   Microphone functions.
*/

bool isAudioPatternOk() {
  if (triggerHistory.size() < SOLUTION_SIZE) {
    return false;
  }

  for (int i = 0; i < SOLUTION_SIZE; i++) {
    if (triggerHistory[i] != SOLUTION_KEY[i]) {
      return false;
    }
  }

  return true;
}

void onMicroChange(int idx, int v, int up) {
  Serial.print(F("Micro :: "));
  Serial.print(idx);
  Serial.print(F(" :: "));
  Serial.println(v);

  if (isTrackPlaying() || progState.isAudioPatternOk) {
    return;
  }

  if (v >= MICROS_THRESHOLD) {
    Serial.println(F("Micro active: Playing audio"));
    triggerHistory.push(idx);
    playTrack(AUDIO_PINS[idx]);
  }
}

void initMicros() {
  for (int i = 0; i < MICROS_NUM; i++) {
    microphones[i]
    .begin(MICRO_PINS[i], MICROS_SAMPLE_RATE_MS)
    .range(MICROS_RANGE_MIN, MICROS_RANGE_MAX)
    .average(MICROS_AVG_BUFS[i], sizeof(MICROS_AVG_BUFS[i]))
    .onChange(onMicroChange, i);
  }
}

/**
   Audio FX functions.
*/

void playTrack(byte trackPin) {
  if (isTrackPlaying()) {
    Serial.println(F("Skipping: Audio still playing"));
    return;
  }

  Serial.print(F("Playing track on pin: "));
  Serial.println(trackPin);

  digitalWrite(trackPin, LOW);
  pinMode(trackPin, OUTPUT);
  delay(300);
  pinMode(trackPin, INPUT);
}

void initAudioPins() {
  for (int i = 0; i < MICROS_NUM; i++) {
    pinMode(AUDIO_PINS[i], INPUT);
  }

  pinMode(PIN_AUDIO_ACT, INPUT);
  pinMode(PIN_AUDIO_RST, INPUT);
}

bool isTrackPlaying() {
  return digitalRead(PIN_AUDIO_ACT) == LOW;
}

void resetAudio() {
  Serial.println(F("Audio FX reset"));

  digitalWrite(PIN_AUDIO_RST, LOW);
  pinMode(PIN_AUDIO_RST, OUTPUT);
  delay(10);
  pinMode(PIN_AUDIO_RST, INPUT);

  Serial.println(F("Waiting for Audio FX startup"));

  delay(2000);
}

/**
   LED functions.
*/

void initLedStrip() {
  ledStrip.begin();
  ledStrip.setBrightness(LED_BRIGHTNESS);
  ledStrip.clear();
  ledStrip.show();
}

void clearLeds() {
  ledStrip.clear();
  ledStrip.show();
}

void startLedSequence(int idx, int v, int up) {
  startLedSequence();
}

void startLedSequence() {
  if (progState.nextLedSequenceStep > -1) {
    Serial.println(F("WARN :: Ongoing LED sequence"));
    return;
  }

  progState.nextLedSequenceStep = 0;
  onLedSequenceStep();
}

void onLedSequenceStep(int idx, int v, int up) {
  onLedSequenceStep();
}

void onLedSequenceStep() {
  if (progState.nextLedSequenceStep < 0) {
    Serial.println(F("WARN :: Next LED sequence step is undefined"));
    return;
  }

  Serial.print(F("LED step: "));
  Serial.println(progState.nextLedSequenceStep);

  clearLeds();

  for (int i = 0; i < LED_NUM; i++) {
    ledStrip.setPixelColor(i, COLOR_SEQUENCE);
  }

  ledStrip.show();

  Serial.print(F("Clearing LEDs in (ms): "));
  Serial.println(SEQ_CLEAR_MS);

  timerLedClear
  .begin(SEQ_CLEAR_MS)
  .repeat(1)
  .onTimer(onLedClear)
  .start();

  int nextStep = progState.nextLedSequenceStep + 1;

  if (nextStep >= KNOCK_PATTERN_SIZE) {
    Serial.println(F("LED sequence finished"));
    progState.nextLedSequenceStep = -1;
    progState.isLedSequenceRunning = false;
    return;
  } else {
    progState.nextLedSequenceStep = nextStep;
  }

  int diffMs = knockPattern[nextStep] - knockPattern[nextStep - 1];

  Serial.print(F("Next LED step in (ms): "));
  Serial.println(diffMs);

  timerLedSequence
  .begin(diffMs)
  .repeat(1)
  .onTimer(onLedSequenceStep)
  .start();
}

void onLedClear(int idx, int v, int up) {
  clearLeds();
}

/**
   Program state functions.
*/

void updateState() {
  if (!progState.isAudioPatternOk && !isAudioPatternOk()) {
    return;
  }

  progState.isAudioPatternOk = true;

  if (!isTrackPlaying() && !progState.hasPlayedFinalAudio) {
    Serial.println(F("Playing final audio"));
    progState.hasPlayedFinalAudio = true;
    playTrack(PIN_AUDIO_FINAL);
  }

  if (!progState.isLedSequenceRunning) {
    Serial.print(F("Scheduling LED sequence in (ms): "));
    Serial.println(LED_SEQ_DELAY_START);

    progState.isLedSequenceRunning = true;

    timerStartSequence
    .begin(LED_SEQ_DELAY_START)
    .repeat(1)
    .onTimer(startLedSequence)
    .start();
  }

  if (!progState.isRelayOpen && isValidKnockPattern()) {
    openRelay();
  }
}

void onStateTimer(int idx, int v, int up) {
  updateState();
}

void initStateTimer() {
  timerState
  .begin(STATE_TIMER_MS)
  .repeat(-1)
  .onTimer(onStateTimer)
  .start();
}

/**
   Relay functions.
*/

void lockRelay() {
  Serial.println(F("Locking relay"));
  digitalWrite(RELAY_PIN, LOW);
  progState.isRelayOpen = false;
}

void openRelay() {
  Serial.println(F("Opening relay"));
  digitalWrite(RELAY_PIN, HIGH);
  progState.isRelayOpen = true;
}

void initRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  lockRelay();
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  initRelay();
  initMicros();
  initLedStrip();
  initAudioPins();
  resetAudio();
  initKnockSensor();
  initStateTimer();

  Serial.println(F(">> Starting Palormonio program"));
  Serial.flush();
}

void loop() {
  automaton.run();
}
