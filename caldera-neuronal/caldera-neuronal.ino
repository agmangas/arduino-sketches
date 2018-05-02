#include <Automaton.h>

typedef struct buttonConfig {
  byte btnPin;
  byte ledPin;
} ButtonConfig;

typedef struct solutionKeyItem {
  bool btnState;
} SolutionKeyItem;

typedef struct programState {
  int currPhase;
  boolean isFinished;
} ProgramState;

const int TOTAL_BUTTONS = 9;
const int NUM_PHASES = 3;

Atm_button atmButtons[TOTAL_BUTTONS];
Atm_led atmLeds[TOTAL_BUTTONS];

ButtonConfig btnConfs[TOTAL_BUTTONS] = {
  { .btnPin = 2, .ledPin = 3 },
  { .btnPin = 4, .ledPin = 5 },
  { .btnPin = 6, .ledPin = 7 },
  { .btnPin = 8, .ledPin = 9 },
  { .btnPin = 10, .ledPin = 11 },
  { .btnPin = 12, .ledPin = 13 },
  { .btnPin = A0, .ledPin = A1 },
  { .btnPin = A2, .ledPin = A3 },
  { .btnPin = A4, .ledPin = A5 }
};

SolutionKeyItem solutionKeys[NUM_PHASES][TOTAL_BUTTONS] = {
  {
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true }
  },
  {
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true }
  },
  {
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true },
    { .btnState = true }
  }
};

ProgramState programState = { .currPhase = 0, .isFinished = false };

boolean getBtnLedState(int btnIndex) {
  return atmLeds[btnIndex].state() !=
         atmLeds[btnIndex].IDLE;
}

boolean isBtnValid(int btnIndex) {
  return solutionKeys[programState.currPhase][btnIndex].btnState ==
         getBtnLedState(btnIndex);
}

boolean isButtonPatternValid() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (!isBtnValid(i)) {
      return false;
    }
  }

  return true;
}

boolean isNextBtnToPress(int btnIndex) {
  int nextIdx = -1;

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (solutionKeys[programState.currPhase][i].btnState == true &&
        getBtnLedState(i) == false) {
      nextIdx = i;
      break;
    }
  }

  return btnIndex == nextIdx;
}

void turnAllLedsOff() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void turnLedOn(int btnIndex) {
  atmLeds[btnIndex].trigger(atmLeds[btnIndex].EVT_ON);
}

void blinkLeds() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].blink(50, 200).trigger(atmLeds[i].EVT_BLINK);
  }
}

boolean isLastPhase() {
  return programState.currPhase == (NUM_PHASES - 1);
}

void onButtonChange(int idx, int v, int up) {
  if (programState.isFinished) {
    return;
  }

  if (isNextBtnToPress(idx)) {
    turnLedOn(idx);
  } else {
    turnAllLedsOff();
  }

  if (!isButtonPatternValid()) {
    return;
  }

  if (isLastPhase()) {
    programState.isFinished = true;
    turnAllLedsOff();
    blinkLeds();
  } else {
    programState.currPhase++;
    turnAllLedsOff();
  }
}

void initButtons() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmButtons[i].begin(btnConfs[i].btnPin).onPress(onButtonChange, i);
    atmLeds[i].begin(btnConfs[i].ledPin);
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void setup() {
  initButtons();
}

void loop() {
  automaton.run();
}
