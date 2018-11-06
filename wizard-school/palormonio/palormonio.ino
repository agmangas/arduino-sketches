#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include "limits.h"

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
   Audio FX.
*/

const byte PIN_AUDIO_RST = 9;
const byte PIN_AUDIO_ACT = 3;

const byte AUDIO_PINS[MICROS_NUM] = {
  8, 7, 6, 5, 4
};

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

  if (v >= MICROS_THRESHOLD) {
    Serial.println(F("# Micro active: Playing audio"));
    playTrack(AUDIO_PINS[idx]);
    waitForAudio();
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

void waitForAudio() {
  const unsigned long AUDIO_WAIT_SLEEP_MS = 10;
  const unsigned long MAX_AUDIO_WAIT_MS = 10000;

  unsigned long ini;
  unsigned long now;
  unsigned long dif;

  Serial.println(F("Waiting for audio track to finish"));

  ini = millis();

  while (isTrackPlaying()) {
    delay(AUDIO_WAIT_SLEEP_MS);

    now = millis();

    if (now > ini) {
      dif = now - ini;
    } else {
      dif = (ULONG_MAX - ini) + now;
    }

    if (dif > MAX_AUDIO_WAIT_MS) {
      Serial.println(F("Max audio wait: Breaking loop"));
      break;
    }
  }
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

void playLedPattern() {
  Serial.println(F("Playing LED pattern"));
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
}
