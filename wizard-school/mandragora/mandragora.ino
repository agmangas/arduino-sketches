#include <Automaton.h>

/**
   LDRs and state machines.
*/

const int LDR_NUM = 3;

const byte LDR_PINS[LDR_NUM] = {
  3, 4, 5
};

const int LDR_PULSE_DURATION = 200;

Atm_digital ldrs[LDR_NUM];
Atm_bit ldrBits[LDR_NUM];

/**
   Audio FX.
*/

const byte PIN_AUDIO_RST = 11;
const byte PIN_AUDIO_ACT = 6;

const byte AUDIO_PINS[LDR_NUM] = {
  10, 9, 8
};

/**
   LDR functions.
*/

void initMachines() {
  for (int i = 0; i < LDR_NUM; i++) {
    ldrBits[i]
    .begin();

    ldrs[i]
    .begin(LDR_PINS[i], LDR_PULSE_DURATION, false, false)
    .onChange(HIGH, ldrBits[i], ldrBits[i].EVT_ON)
    .onChange(LOW, ldrBits[i], ldrBits[i].EVT_OFF);
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
  for (int i = 0; i < LDR_NUM; i++) {
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
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  initMachines();
  initAudioPins();
  resetAudio();

  Serial.println(">> Starting Mandragora program");
}

void loop() {
  automaton.run();
}

