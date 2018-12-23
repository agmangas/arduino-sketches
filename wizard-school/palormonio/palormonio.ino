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
  bool isRelayOpen;
  bool isLedSequenceRunning;
  unsigned long ledSeqStartMillis;
  unsigned long lastLedSeqMillis;
} ProgramState;

ProgramState progState = {
  .isAudioPatternOk = false,
  .hasPlayedFinalAudio = false,
  .lastKnock = 0,
  .isRelayOpen = false,
  .isLedSequenceRunning = false,
  .ledSeqStartMillis = 0,
  .lastLedSeqMillis = 0
};

Atm_timer timerState;

const int STATE_TIMER_MS = 60;

/**
   Relay.
*/

const byte RELAY_PIN = 12;

/**
   Piezo knock sensor.
*/

const byte KNOCK_PIN = A5;
const byte KNOCK_THRESHOLD = 30;
const byte KNOCK_SAMPLERATE = 50;
const byte KNOCK_BUF_SIZE = 30;
const byte KNOCK_TRAINING_SIZE = 5;
const byte KNOCK_PATTERN_SIZE = 5;
const float KNOCK_TOLERANCE = 0.4;
const unsigned int KNOCK_DELAY_MS = 150;

unsigned int knockTrainingSet[KNOCK_TRAINING_SIZE][KNOCK_PATTERN_SIZE] = {
  {0, 478, 1279, 1609, 2292},
  {0, 595, 1448, 1775, 2354},
  {0, 596, 1446, 1751, 2409},
  {0, 621, 1549, 1840, 2462},
  {0, 556, 1542, 1846, 2477}
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
const int MICROS_SAMPLE_RATE_MS = 20;
const int MICROS_RANGE_MIN = 0;
const int MICROS_RANGE_MAX = 100;
const int MICROS_THRESHOLD = 8;

const byte MICRO_PINS[MICROS_NUM] = {
  A0, A1, A2, A3, A4
};

uint16_t MICROS_AVG_BUFS[MICROS_NUM][MICROS_AVG_BUF_SIZE];

Atm_analog microphones[MICROS_NUM];

/**
   Solution key.
*/

const int SOLUTION_SIZE = 5;

CircularBuffer<byte, SOLUTION_SIZE> triggerHistory;

const byte SOLUTION_KEY[SOLUTION_SIZE] = {
  0, 2, 4, 3, 2
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

const int LED_BRIGHTNESS = 150;
const int LED_PIN = 11;
const int LED_NUM = 30;
const uint32_t COLOR_SEQUENCE = Adafruit_NeoPixel::Color(100, 255, 0);
const int LED_SEQ_CLEAR_MS = 200;
const int LED_SEQ_DELAY_MS = 5000;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

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

  int maxOffset = knockHistory.size() - KNOCK_PATTERN_SIZE;

  for (int i = 0; i <= maxOffset; i++) {
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
    Serial.println(F("::K"));
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
  Serial.print(F("M:"));
  Serial.print(idx);
  Serial.print(F(":"));
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

  resetLedSequence();
}

void clearLeds() {
  ledStrip.clear();
  ledStrip.show();
}

void startLedSequence(int idx, int v, int up) {
  startLedSequence();
}

void startLedSequence() {
  if (progState.isLedSequenceRunning) {
    Serial.println(F("WARN :: LED sequence ongoing"));
    return;
  }

  Serial.println(F("Starting LED sequence"));
  progState.isLedSequenceRunning = true;
  progState.lastLedSeqMillis = millis();
}

void resetLedSequence() {
  clearLeds();
  progState.ledSeqStartMillis = 0;
  progState.isLedSequenceRunning = false;
}

void showLedSequence() {
  for (int i = 0; i < LED_NUM; i++) {
    ledStrip.setPixelColor(i, COLOR_SEQUENCE);
  }

  ledStrip.show();
}

void updateLedSequence(int idx, int v, int up) {
  updateLedSequence();
}

void updateLedSequence() {
  if (!progState.isAudioPatternOk ||
      !progState.isLedSequenceRunning) {
    clearLeds();
    return;
  }

  unsigned long now = millis();

  if (progState.ledSeqStartMillis == 0) {
    Serial.println(F("Updating LED sequence start stamp"));
    progState.ledSeqStartMillis = now;
  }

  if (now < progState.ledSeqStartMillis) {
    Serial.println(F("Millis overflow on LED sequence loop"));
    resetLedSequence();
    return;
  }

  unsigned long diff = now - progState.ledSeqStartMillis;

  float limitMillisSoft = knockPattern[KNOCK_PATTERN_SIZE - 1];
  float limitMillisHard = limitMillisSoft + LED_SEQ_CLEAR_MS;

  if (diff >= limitMillisSoft) {
    if (diff < limitMillisHard) {
      showLedSequence();
    } else {
      resetLedSequence();
    }

    return;
  }

  int currIdx = -1;

  for (int i = 0; i < (KNOCK_PATTERN_SIZE - 1); i++) {
    if (diff >= knockPattern[i] &&
        diff < knockPattern[i + 1]) {
      currIdx = i;
      break;
    }
  }

  if (currIdx == -1) {
    Serial.println(F("WARN :: Unable to find LED sequence index"));
    resetLedSequence();
    return;
  }

  float limitMillisShow = knockPattern[currIdx] + LED_SEQ_CLEAR_MS;

  if (diff < limitMillisShow) {
    showLedSequence();
  } else {
    clearLeds();
  }
}

bool isLedSequenceStartAllowed() {
  if (progState.isLedSequenceRunning) {
    return false;
  }

  if (progState.lastLedSeqMillis == 0) {
    return true;
  }

  unsigned long now = millis();

  if (now < progState.lastLedSeqMillis) {
    Serial.println(F("Overflow checking LED schedule"));
    progState.lastLedSeqMillis = 0;
    return false;
  }

  unsigned long diff = now - progState.lastLedSeqMillis;
  unsigned long minDiff = LED_SEQ_DELAY_MS + knockPattern[KNOCK_PATTERN_SIZE - 1];

  return diff >= minDiff;
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

  if (isLedSequenceStartAllowed()) {
    startLedSequence();
  }

  updateLedSequence();

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
