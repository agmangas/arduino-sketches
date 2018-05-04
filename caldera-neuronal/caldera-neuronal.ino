#include <Automaton.h>

typedef struct buttonConfig {
  byte btnPin;
  byte ledPin;
} ButtonConfig;

typedef struct solutionKeyItem {
  bool btnState;
  int btnIndex;
} SolutionKeyItem;

typedef struct programState {
  int currPhase;
  boolean isStarted;
  boolean isFinished;
} ProgramState;

const int TOTAL_BUTTONS = 8;
const int NUM_PHASES = 3;
const unsigned long PATTERN_LONG_MS = 400;
const unsigned long PATTERN_SHORT_MS = 150;

Atm_button atmButtons[TOTAL_BUTTONS];
Atm_led atmLeds[TOTAL_BUTTONS];

ButtonConfig btnConfs[TOTAL_BUTTONS] = {
  { .btnPin = 4, .ledPin = 5 },
  { .btnPin = 6, .ledPin = 7 },
  { .btnPin = 8, .ledPin = 9 },
  { .btnPin = 10, .ledPin = 11 },
  { .btnPin = 12, .ledPin = 13 },
  { .btnPin = A0, .ledPin = A1 },
  { .btnPin = A2, .ledPin = A3 },
  { .btnPin = A4, .ledPin = A5 }
};

/**
   This array contains the solution keys for each phase.
   Each phase subarray should contain the solution states for each
   button in the sequence (i.e. its size should be TOTAL_BUTTONS).
   The order of the items in the phase subarray and the value of the
   btnIndex member determines the order in which the buttons should be
   pressed (if btnState is true, otherwise the button will be ignored).
*/
SolutionKeyItem solutionKeys[NUM_PHASES][TOTAL_BUTTONS] = {
  {
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 7 }
  },
  {
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 7 }
  },
  {
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 7 }
  }
};

ProgramState programState = {
  .currPhase = 0,
  .isStarted = false,
  .isFinished = false
};

boolean getBtnLedState(int btnIndex) {
  return atmLeds[btnIndex].state() != atmLeds[btnIndex].IDLE;
}

SolutionKeyItem* findSolutionKeyItem(int btnIndex) {
  SolutionKeyItem* currPhaseSolution = solutionKeys[programState.currPhase];

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    if (currPhaseSolution[i].btnIndex == btnIndex) {
      return currPhaseSolution + i;
    }
  }

  return NULL;
}

boolean isButtonPatternValid() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    SolutionKeyItem* btnSolution = findSolutionKeyItem(i);

    // Value should never be NULL
    if (btnSolution == NULL || btnSolution->btnState != getBtnLedState(i)) {
      return false;
    }
  }

  return true;
}

boolean isNextBtnToPress(int btnIndex) {
  SolutionKeyItem* currPhaseSolution = solutionKeys[programState.currPhase];

  int nextIndex = -1;

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    boolean btnLedState = getBtnLedState(currPhaseSolution[i].btnIndex);
    boolean btnSolutionState = currPhaseSolution[i].btnState;

    if (btnSolutionState == true && btnLedState == false) {
      nextIndex = i;
      break;
    }
  }

  return nextIndex == btnIndex;
}

void playSolutionPattern(int phase, unsigned long delayMs) {
  if (phase >= NUM_PHASES || phase < 0) {
    return;
  }

  turnAllLedsOff();

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    int btnIndex = solutionKeys[phase][i].btnIndex;
    atmLeds[btnIndex].trigger(atmLeds[i].EVT_ON);
    delay(delayMs);
  }

  turnAllLedsOff();
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
    atmLeds[i].blink(50, 350).trigger(atmLeds[i].EVT_BLINK);
  }
}

boolean isLastPhase() {
  return programState.currPhase == (NUM_PHASES - 1);
}

void onButtonChange(int idx, int v, int up) {
  if (!programState.isStarted) {
    programState.isStarted = true;
    playSolutionPattern(0, PATTERN_LONG_MS);
  }

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
    playSolutionPattern(programState.currPhase, PATTERN_SHORT_MS);
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

