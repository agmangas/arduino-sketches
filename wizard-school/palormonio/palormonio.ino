#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>
#include "limits.h"

/**
  Structs.
*/

typedef struct programState {
  bool isComplete;
  bool hasPlayedFinalAudio;
} ProgramState;

ProgramState progState = {
  .isComplete = false,
  .hasPlayedFinalAudio = false
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

const uint32_t COLOR_BLUE = Adafruit_NeoPixel::Color(0, 0, 255);
const uint32_t COLOR_RED = Adafruit_NeoPixel::Color(255, 0, 0);

const unsigned long DELAY_SHORT = 300;
const unsigned long DELAY_LONG = 1000;
const unsigned long DELAY_BEFORE_SEQUENCE = 3000;
const unsigned long DEFAULT_LIGHT_DELAY = 300;

const int SEQUENCE_SIZE = 7;

const LedSequenceStep ledSeqSteps[SEQUENCE_SIZE] = {
  {
    .afterDelay = DELAY_SHORT,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_BLUE
  },
  {
    .afterDelay = DELAY_SHORT,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_BLUE
  },
  {
    .afterDelay = DELAY_LONG,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_RED
  },
  {
    .afterDelay = DELAY_SHORT,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_BLUE
  },
  {
    .afterDelay = DELAY_SHORT,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_BLUE
  },
  {
    .afterDelay = DELAY_LONG,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_RED
  },
  {
    .afterDelay = DELAY_LONG,
    .lightDelay = DEFAULT_LIGHT_DELAY,
    .color = COLOR_RED
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

const int LED_BRIGHTNESS = 170;
const int LED_PIN = 11;
const int LED_NUM = 30;

const unsigned long LED_FADEIN_STEP_MS = 10;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
   Microphone functions.
*/

void onMicroChange(int idx, int v, int up) {
  Serial.print(F("# Micro :: "));
  Serial.print(idx);
  Serial.print(F(" :: "));
  Serial.println(v);

  if (isTrackPlaying() || progState.isComplete) {
    return;
  }

  if (v >= MICROS_THRESHOLD) {
    Serial.println(F("# Micro active: Playing audio"));
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
    Serial.println(F("# Skipping: Audio still playing"));
    return;
  }

  Serial.print(F("# Playing track on pin: "));
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
  Serial.println(F("# Audio FX reset"));

  digitalWrite(PIN_AUDIO_RST, LOW);
  pinMode(PIN_AUDIO_RST, OUTPUT);
  delay(10);
  pinMode(PIN_AUDIO_RST, INPUT);

  Serial.println(F("# Waiting for Audio FX startup"));

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

void ledFadeIn() {
  clearLeds();

  for (int i = 0; i < 200; i++) {
    for (int j = 0; j < LED_NUM; j++) {
      ledStrip.setPixelColor(j, i, i, i);
    }

    ledStrip.show();

    delay(LED_FADEIN_STEP_MS);
  }

  clearLeds();
}

void playLedSequence() {
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
}

/**
   Program state functions.
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

void applyCompletionStatus() {
  bool isComplete = isPatternOk() || progState.isComplete;

  if (isTrackPlaying() || !isComplete) {
    return;
  }

  progState.isComplete = true;

  if (!progState.hasPlayedFinalAudio) {
    progState.hasPlayedFinalAudio = true;
    playTrack(PIN_AUDIO_FINAL);
  }

  ledFadeIn();
  delay(DELAY_BEFORE_SEQUENCE);
  playLedSequence();
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
  applyCompletionStatus();
}
