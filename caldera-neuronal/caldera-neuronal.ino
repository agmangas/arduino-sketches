#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

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
  boolean isActive;
  boolean isFinished;
} ProgramState;

typedef struct nextButton {
  int sequenceIndex;
  int btnIndex;
} NextButton;

const int TOTAL_BUTTONS = 8;
const int NUM_PHASES = 3;

const uint16_t NEOPIXEL_NUM = 33;
const uint8_t NEOPIXEL_PIN = 3;

const int NUM_LEDS_PROGRESS = 8;
const int NUM_LEDS_SOLUTION = 3;

const unsigned long PATTERN_MS = 500;

const unsigned long BLINK_DELAY_MS = 8000;

Atm_button atmButtons[TOTAL_BUTTONS];
Atm_led atmLeds[TOTAL_BUTTONS];
Atm_controller startupController;
Atm_timer timer;

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
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 0 }
  },
  {
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 6 }
  },
  {
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 0 }
  }
};

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t solutionColors[NUM_PHASES] = {
  pixelStrip.Color(255, 0, 0),
  pixelStrip.Color(0, 255, 0),
  pixelStrip.Color(0, 0, 255)
};

const uint32_t PROGRESS_COLOR = pixelStrip.Color(5, 0, 5);

ProgramState programState = {
  .currPhase = 0,
  .isActive = false,
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

NextButton getNextButton() {
  NextButton nxtBtn = { .sequenceIndex = -1, .btnIndex = -1 };

  SolutionKeyItem* currPhaseSolution = solutionKeys[programState.currPhase];

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    boolean btnLedState = getBtnLedState(currPhaseSolution[i].btnIndex);
    boolean btnSolutionState = currPhaseSolution[i].btnState;

    if (btnSolutionState == true && btnLedState == false) {
      nxtBtn.sequenceIndex = i;
      nxtBtn.btnIndex = currPhaseSolution[i].btnIndex;
      break;
    }
  }

  return nxtBtn;
}

void playSolutionPattern(int phase, unsigned long delayMs) {
  if (phase >= NUM_PHASES || phase < 0) {
    return;
  }

  turnAllButtonLedsOff();

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    int btnIndex = solutionKeys[phase][i].btnIndex;
    atmLeds[btnIndex].trigger(atmLeds[i].EVT_ON);
    delay(delayMs);
  }

  turnAllButtonLedsOff();
}

void turnAllButtonLedsOff() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void turnButtonLedOn(int btnIndex) {
  atmLeds[btnIndex].trigger(atmLeds[btnIndex].EVT_ON);
}

void blinkButtonLeds() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].blink(50, 350).trigger(atmLeds[i].EVT_BLINK);
  }
}

void stopBlinkButtonLeds() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void turnAllStripLedsOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void activateSolutionStripLeds(int phaseIdx) {
  if (phaseIdx < 0 || phaseIdx >= NUM_PHASES) {
    return;
  }

  int loIdx = NUM_LEDS_PROGRESS * 3  + NUM_LEDS_SOLUTION * phaseIdx;
  int hiIdx = loIdx + NUM_LEDS_SOLUTION;

  for (int i = loIdx; i < hiIdx; i++) {
    pixelStrip.setPixelColor(i, solutionColors[phaseIdx]);
  }

  pixelStrip.show();
}

void activateProgressStripLeds(int phaseIdx, int level) {
  if (phaseIdx < 0 || phaseIdx >= NUM_PHASES) {
    return;
  }

  if (level < 0 || level >= TOTAL_BUTTONS) {
    return;
  }

  int loIdx = NUM_LEDS_PROGRESS * phaseIdx;
  int hiIdx = loIdx + (level + 1);

  for (int i = loIdx; i < hiIdx; i++) {
    pixelStrip.setPixelColor(i, PROGRESS_COLOR);
  }

  pixelStrip.show();
}

void turnProgressStripLedsOff(int phaseIdx) {
  if (phaseIdx < 0 || phaseIdx >= NUM_PHASES) {
    return;
  }

  int loIdx = NUM_LEDS_PROGRESS * phaseIdx;
  int hiIdx = loIdx + TOTAL_BUTTONS;

  for (int i = loIdx; i < hiIdx; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

boolean isLastPhase() {
  return programState.currPhase == (NUM_PHASES - 1);
}

void onButtonChange(int idx, int v, int up) {
  if (!programState.isActive) {
    Serial.println("isActive = False: Ignoring button press");
    return;
  }

  if (programState.isFinished) {
    Serial.println("Program has finished: ignoring button event");
    return;
  }

  NextButton nxtBtn = getNextButton();

  boolean isNextBtnToPress = nxtBtn.btnIndex != -1 &&
                             nxtBtn.btnIndex == idx;

  Serial.print("Next button to press: btnIndex=");
  Serial.print(nxtBtn.btnIndex);
  Serial.print(" sequenceIndex=");
  Serial.print(nxtBtn.sequenceIndex);
  Serial.println();

  if (isNextBtnToPress) {
    Serial.println("Is next button: Activating strip and LED");
    activateProgressStripLeds(programState.currPhase, nxtBtn.sequenceIndex);
    turnButtonLedOn(idx);
  } else {
    Serial.println("Not the next button: Turning off strip and LED");
    turnProgressStripLedsOff(programState.currPhase);
    turnAllButtonLedsOff();
  }

  if (!isButtonPatternValid()) {
    return;
  }

  if (isLastPhase()) {
    programState.isFinished = true;
    turnAllButtonLedsOff();
    blinkButtonLeds();

    for (int i = 0; i < NUM_PHASES; i++) {
      activateSolutionStripLeds(i);
    }
  } else {
    programState.currPhase++;
    turnAllButtonLedsOff();
    playSolutionPattern(programState.currPhase, PATTERN_MS);
  }
}

void startProgram() {
  if (programState.isActive) {
    return;
  }

  Serial.println("Setting isActive = true and playing first pattern");

  blinkButtonLeds();

  timer.begin(BLINK_DELAY_MS)
  .onTimer([] (int idx, int v, int up) {
    stopBlinkButtonLeds();
    playSolutionPattern(0, PATTERN_MS);
    programState.isActive = true;
  })
  .start();
}

void initMachines() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmButtons[i].begin(btnConfs[i].btnPin).onPress(onButtonChange, i);
    atmLeds[i].begin(btnConfs[i].ledPin);
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }

  startupController.begin()
  .IF(atmButtons[1], '=', atmButtons[1].PRESSED)
  .IF(atmButtons[7], '=', atmButtons[7].PRESSED)
  .onChange(true, startProgram);
}

void setup() {
  Serial.begin(9600);

  pixelStrip.begin();
  pixelStrip.setBrightness(250);
  pixelStrip.show();

  initMachines();

  turnAllStripLedsOff();

  Serial.println(">> Starting Caldera Neuronal program");
}

void loop() {
  automaton.run();
}
