#include <Automaton.h>
#include <CircularBuffer.h>

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
Atm_timer ldrSnapshotTimer;

/**
  Buffer of LDR snapshots.
*/

typedef struct ldrSnapshot {
  bool states[LDR_NUM];
} LdrSnapshot;

const int LDR_SNAPSHOT_BUF_SIZE = 50;
const int LDR_SNAPSHOT_MS = 200;

CircularBuffer<LdrSnapshot, LDR_SNAPSHOT_BUF_SIZE> ldrSnapBuf;

const int LDR_SNAPSHOT_SOLUTION_SIZE = 4;

const LdrSnapshot ldrSnapSolution[LDR_SNAPSHOT_SOLUTION_SIZE] = {
  {.states = {false, false, false}},
  {.states = {false, false, true}},
  {.states = {false, true, true}},
  {.states = {true, true, true}}
};

/**
   Audio FX.
*/

const byte AUDIO_PIN_RST = 11;
const byte AUDIO_PIN_ACT = 6;
const byte AUDIO_PIN_FINAL = 7;

const int AUDIO_NUM_TRACKS = 3;

const byte AUDIO_PINS[AUDIO_NUM_TRACKS] = {
  10, 9, 8
};

/**
   LDR functions.
*/

bool isValidLdrSnapHistory() {
  if (ldrSnapBuf.size() < LDR_SNAPSHOT_SOLUTION_SIZE) {
    return false;
  }

  int iniIdx = ldrSnapBuf.size() - LDR_SNAPSHOT_SOLUTION_SIZE;
  int endIdx = ldrSnapBuf.size();

  int pivot = 0;

  for (int i = iniIdx; i < endIdx; i++) {
    if (!equalLdrSnaps(ldrSnapBuf[i], ldrSnapSolution[pivot])) {
      return false;
    }

    pivot++;
  }

  return true;
}

bool equalLdrSnaps(const LdrSnapshot &one, const LdrSnapshot &other) {
  for (int i = 0; i < LDR_NUM; i++) {
    if (one.states[i] != other.states[i]) {
      return false;
    }
  }

  return true;
}

void takeLdrSnapshot(int idx, int v, int up) {
  LdrSnapshot currSnapshot;

  for (int i = 0; i < LDR_NUM; i++) {
    currSnapshot.states[i] = ldrBits[i].state() == 1;
  }

  bool shouldPush = ldrSnapBuf.isEmpty() ||
                    !equalLdrSnaps(currSnapshot, ldrSnapBuf.last());

  if (shouldPush) {
    Serial.print(F("Pushing LDR snapshot: "));

    for (int i = 0; i < LDR_NUM; i++) {
      Serial.print(currSnapshot.states[i]);
    }

    Serial.println();

    ldrSnapBuf.push(currSnapshot);
  }
}

/**
   State machine functions.
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

  ldrSnapshotTimer
  .begin(LDR_SNAPSHOT_MS)
  .repeat(-1)
  .onTimer(takeLdrSnapshot)
  .start();
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

  pinMode(AUDIO_PIN_ACT, INPUT);
  pinMode(AUDIO_PIN_RST, INPUT);
}

bool isTrackPlaying() {
  return digitalRead(AUDIO_PIN_ACT) == LOW;
}

void resetAudio() {
  digitalWrite(AUDIO_PIN_RST, LOW);
  pinMode(AUDIO_PIN_RST, OUTPUT);
  delay(10);
  pinMode(AUDIO_PIN_RST, INPUT);
  delay(1000);
}

void checkStatusToPlayAudio() {
  if (ldrSnapBuf.isEmpty() || isTrackPlaying()) {
    return;
  }

  LdrSnapshot defaultSnap = {
    .states = {false, false, false}
  };

  if (equalLdrSnaps(defaultSnap, ldrSnapBuf.last())) {
    return;
  }

  if (isValidLdrSnapHistory()) {
    playTrack(AUDIO_PIN_FINAL);
  } else {
    long randAudioIdx = random(0, AUDIO_NUM_TRACKS);
    playTrack(AUDIO_PINS[randAudioIdx]);
  }
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
  checkStatusToPlayAudio();
}

