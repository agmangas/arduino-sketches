#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>
#include "limits.h"

/**
  Structs.
*/

typedef struct programState {
  bool isAllowedToPlay;
} ProgramState;

ProgramState progState = {
  .isAllowedToPlay = true
};

typedef struct ledSequenceStep {
  unsigned long afterDelay;
  unsigned long lightDelay;
  uint32_t color;
} LedSequenceStep;

/**
   Microphones.
*/

const int MICROS_NUM = 5;
const int MICROS_AVG_BUF_SIZE = 5;
const int MICROS_SAMPLE_RATE_MS = 50;
const int MICROS_RANGE_MIN = 1;
const int MICROS_RANGE_MAX = 1000;
const int MICROS_THRESHOLD = 850;

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

const unsigned long SEQUENCE_DEFAULT_LIGHT_MS = 200;
const uint32_t SEQUENCE_DEFAULT_COLOR = Adafruit_NeoPixel::Color(0, 0, 255);
const int SEQUENCE_SIZE = 5;

const LedSequenceStep ledSeqSteps[SEQUENCE_SIZE] = {
  {
    .afterDelay = 500,
    .lightDelay = SEQUENCE_DEFAULT_LIGHT_MS,
    .color = SEQUENCE_DEFAULT_COLOR
  },
  {
    .afterDelay = 1000,
    .lightDelay = SEQUENCE_DEFAULT_LIGHT_MS,
    .color = SEQUENCE_DEFAULT_COLOR
  },
  {
    .afterDelay = 500,
    .lightDelay = SEQUENCE_DEFAULT_LIGHT_MS,
    .color = SEQUENCE_DEFAULT_COLOR
  },
  {
    .afterDelay = 500,
    .lightDelay = SEQUENCE_DEFAULT_LIGHT_MS,
    .color = SEQUENCE_DEFAULT_COLOR
  },
  {
    .afterDelay = 1000,
    .lightDelay = SEQUENCE_DEFAULT_LIGHT_MS,
    .color = SEQUENCE_DEFAULT_COLOR
  }
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

const int LED_BRIGHTNESS = 180;
const int LED_PIN = 11;
const int LED_NUM = 10;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
   Microphone functions.
*/

void onMicroChange(int idx, int v, int up) {
  Serial.print(F("# Micro :: "));
  Serial.print(idx);
  Serial.print(F(" :: "));
  Serial.println(v);

  if (isTrackPlaying()) {
    Serial.println(F("# Audio still playing: Skipping"));
    return;
  }

  if (v >= MICROS_THRESHOLD) {
    Serial.println(F("# Micro active: Playing audio"));
    triggerHistory.push(idx);
    progState.isAllowedToPlay = true;
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
  digitalWrite(trackPin, LOW);
  pinMode(trackPin, OUTPUT);
  delay(100);
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
  digitalWrite(PIN_AUDIO_RST, LOW);
  pinMode(PIN_AUDIO_RST, OUTPUT);
  delay(10);
  pinMode(PIN_AUDIO_RST, INPUT);
  delay(1000);
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

void checkStatusPlayLedSequence() {
  if (isTrackPlaying()) {
    return;
  }

  if (isPatternOk() == false || progState.isAllowedToPlay == false) {
    return;
  }

  playTrack(PIN_AUDIO_FINAL);

  for (int i = 0; i < SEQUENCE_SIZE; i++) {
    clearLeds();

    for (int j = 0; j < LED_NUM; j++) {
      ledStrip.setPixelColor(j, ledSeqSteps[i].color);
    }

    ledStrip.show();

    delay(ledSeqSteps[i].lightDelay);

    clearLeds();

    delay(ledSeqSteps[i].afterDelay);
  }

  clearLeds();

  progState.isAllowedToPlay = false;
}

/**
   Other functions.
*/

bool isPatternOk() {
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

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  initMicros();
  initLedStrip();
  initAudioPins();
  resetAudio();

  Serial.println(F(">> Starting Palormonio program"));
  Serial.flush();
}

void loop() {
  automaton.run();
  clearLeds();
  checkStatusPlayLedSequence();
}
