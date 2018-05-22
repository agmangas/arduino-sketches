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
  boolean isStarted;
  boolean isBlinking;
  boolean isFinished;
} ProgramState;

typedef struct nextButton {
  int sequenceIndex;
  int btnIndex;
} NextButton;

const int TOTAL_BUTTONS = 8;
const int NUM_PHASES = 3;

const byte PIN_AUDIO_TRACK_PHASE = 2;
const byte PIN_AUDIO_TRACK_COMPLETION = 1;
const byte PIN_AUDIO_TRACK_FAIL = 0;

const uint16_t NEOPIXEL_NUM = 33;
const uint8_t NEOPIXEL_PIN = 3;

const int NUM_LEDS_PROGRESS = 8;
const int NUM_LEDS_SOLUTION = 3;

const unsigned long PATTERN_MS = 500;
const unsigned long BLINK_DELAY_MS = 10000;
const unsigned long AFTER_BLINK_DELAY_MS = 2000;

const int START_BTN_0 = 1;
const int START_BTN_1 = 7;

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
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 6 }
  },
  {
    { .btnState = true, .btnIndex = 1 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 0 }
  },
  {
    { .btnState = true, .btnIndex = 6 },
    { .btnState = true, .btnIndex = 7 },
    { .btnState = true, .btnIndex = 4 },
    { .btnState = true, .btnIndex = 5 },
    { .btnState = true, .btnIndex = 2 },
    { .btnState = true, .btnIndex = 3 },
    { .btnState = true, .btnIndex = 0 },
    { .btnState = true, .btnIndex = 1 }
  }
};

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t solutionColors[NUM_PHASES] = {
  pixelStrip.Color(255, 0, 0),
  pixelStrip.Color(0, 255, 0),
  pixelStrip.Color(0, 0, 255)
};

const uint32_t PROGRESS_COLOR = pixelStrip.Color(15, 0, 15);

const int TRACK_NAME_LEN = 12;

char startTrack[TRACK_NAME_LEN] = "START000OGG";
char failTrack[TRACK_NAME_LEN] = "FAIL0000OGG";

char postPhaseTracks[NUM_PHASES][TRACK_NAME_LEN] = {
  "PHASE001OGG",
  "PHASE002OGG",
  "PHASE003OGG"
};

ProgramState programState = {
  .currPhase = 0,
  .isStarted = false,
  .isBlinking = false,
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

void playSolutionPattern() {
  setButtonLedsOff();

  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    int btnIndex = solutionKeys[programState.currPhase][i].btnIndex;
    atmLeds[btnIndex].trigger(atmLeds[i].EVT_ON);
    delay(PATTERN_MS);
  }

  setButtonLedsOff();
}

void setButtonLedsOff() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }
}

void setButtonLedOn(int btnIndex) {
  atmLeds[btnIndex].trigger(atmLeds[btnIndex].EVT_ON);
}

void startBlink() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmLeds[i].blink(50, 350).trigger(atmLeds[i].EVT_BLINK);
  }
}

void blinkAndPlaySolutionPattern() {
  if (programState.isBlinking) {
    return;
  }

  programState.isBlinking = true;

  startBlink();

  timer.begin(BLINK_DELAY_MS)
  .onFinish([] (int idx, int v, int up) {
    setButtonLedsOff();
    delay(AFTER_BLINK_DELAY_MS);
    playSolutionPattern();
    programState.isBlinking = false;
  })
  .start();
}

void setStripOff() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

void setSolutionStripOn(int phaseIdx) {
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

void setProgressStripOn(int phaseIdx, int level) {
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

void setProgressStripOff(int phaseIdx) {
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
  if (programState.isBlinking || !programState.isStarted || programState.isFinished) {
    return;
  }

  NextButton nxtBtn = getNextButton();

  boolean isNextBtnToPress = nxtBtn.btnIndex != -1 &&
                             nxtBtn.btnIndex == idx;

  if (isNextBtnToPress) {
    setProgressStripOn(programState.currPhase, nxtBtn.sequenceIndex);
    setButtonLedOn(idx);
  } else {
    playTrack(PIN_AUDIO_TRACK_FAIL);
    setProgressStripOff(programState.currPhase);
    setButtonLedsOff();
  }

  if (!isButtonPatternValid()) {
    return;
  }

  if (isLastPhase()) {
    programState.isFinished = true;
    playTrack(PIN_AUDIO_TRACK_COMPLETION);
    setButtonLedsOff();
    startBlink();

    for (int i = 0; i < NUM_PHASES; i++) {
      setSolutionStripOn(i);
    }
  } else {
    programState.currPhase++;
    setButtonLedsOff();
    playTrack(PIN_AUDIO_TRACK_PHASE);
    blinkAndPlaySolutionPattern();
  }
}

void startProgram(int idx, int v, int up) {
  if (programState.isStarted) {
    return;
  }

  playTrack(PIN_AUDIO_TRACK_PHASE);

  programState.isStarted = true;
  blinkAndPlaySolutionPattern();
}

void playTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

void initMachines() {
  for (int i = 0; i < TOTAL_BUTTONS; i++) {
    atmButtons[i].begin(btnConfs[i].btnPin).onPress(onButtonChange, i);
    atmLeds[i].begin(btnConfs[i].ledPin);
    atmLeds[i].trigger(atmLeds[i].EVT_OFF);
  }

  startupController.begin()
  .IF(atmButtons[START_BTN_0], '=', atmButtons[START_BTN_0].PRESSED)
  .IF(atmButtons[START_BTN_1], '=', atmButtons[START_BTN_1].PRESSED)
  .onChange(true, startProgram);
}

void initAudioPins() {
  pinMode(PIN_AUDIO_TRACK_PHASE, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_PHASE, HIGH);

  pinMode(PIN_AUDIO_TRACK_COMPLETION, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_COMPLETION, HIGH);

  pinMode(PIN_AUDIO_TRACK_FAIL, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_FAIL, HIGH);
}

void setup() {
  pixelStrip.begin();
  pixelStrip.setBrightness(250);
  pixelStrip.show();

  initMachines();
  setStripOff();
  initAudioPins();
}

void loop() {
  automaton.run();
}
