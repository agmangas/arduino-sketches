#include <Automaton.h>

typedef struct buttonConfig {
  byte btnPin;
  byte ledPin;
} ButtonConfig;

typedef struct solutionKeyItem {
  bool btnState;
} SolutionKeyItem;

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

unsigned long currPhase = 0;

boolean getBtnLedState(byte btnIndex) {
  return atmLeds[btnIndex].state() != atmLeds[btnIndex].IDLE;
}

boolean isBtnValid(byte btnIndex) {
  return solutionKeys[currPhase][btnIndex].btnState == getBtnLedState(btnIndex);
}

boolean isButtonPatternValid() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (!isBtnValid(i)) {
      return false;
    }
  }

  return true;
}

int getNextBtnIndexToPress() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (solutionKeys[currPhase][i].btnState == true &&
        getBtnLedState(i) == false) {
      return i;
    }
  }

  return -1;
}

void resetBtnLeds() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void onButtonChange(int idx, int v, int up) {
  int nextIdx = getNextBtnIndexToPress();

  if (idx != nextIdx) {
    resetBtnLeds();
  } else {
    atmLeds[idx].trigger(atmLeds[idx].EVT_TOGGLE);
  }

  if (isButtonPatternValid()) {
    currPhase++;
    resetBtnLeds();
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
