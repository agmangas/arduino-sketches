#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

typedef struct programState {
  bool solvedDigitalSwitches;
  bool solvedButtons;
  bool solvedPots;
} ProgramState;

/**
   Digital switches
*/

const int NUM_DIGITAL_SWITCH = 4;

int digitalSwitchPins[NUM_DIGITAL_SWITCH] = {
  10, 11, 12, A0
};

Atm_button digitalSwitches[NUM_DIGITAL_SWITCH];

/**
   Buttons
*/

const int NUM_BUTTONS = 7;

int buttonPins[NUM_BUTTONS] = {
  9, 3, 4, 5, 6, 7, 8
};

bool buttonsPattern[NUM_BUTTONS] = {
  true, false, true, false, true, false, true
};

Atm_button buttons[NUM_BUTTONS];
Atm_bit buttonBits[NUM_BUTTONS];

/**
   Potentiometers
*/

const int NUM_POTS = 4;

int potPins[NUM_POTS] = {
  A4, A3, A2, A1
};

Atm_analog pots[NUM_POTS];

const byte POTS_RANGE_LO = 0;
const byte POTS_RANGE_HI = 4;

const int DELAY_CHECK_MS = 50;
const int NUM_CHECKS = 10;

const int NUM_POT_PATTERNS = 2;

byte potPatterns[NUM_POT_PATTERNS][NUM_POTS] = {
  {1, 3, 0, 2},
  {1, 4, 0, 2}
};

/**
   LED
*/

const uint16_t NEOPIXEL_NUM = 244;
const uint8_t NEOPIXEL_PIN = 5;

/**
   Program state
*/

ProgramState progState = {
  .solvedDigitalSwitches = false,
  .solvedButtons = false
};

/**
   Button functions
*/

void onBitChange(int idx, int v, int up) {
  Serial.print("Bit:");
  Serial.print(idx);
  Serial.print(":");
  Serial.println(buttonBits[idx].state());
}

void onCorrectButtonsCombi() {
  if (progState.solvedButtons) {
    return;
  }

  Serial.println("Btn:V");
  progState.solvedButtons = true;
}

bool isCorrectButtonsCombi() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonBits[i].state() == 0 &&
        buttonsPattern[i] == true) {
      return false;
    }
  }

  return true;
}

void initButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonBits[i]
    .begin(false)
    .onChange(onBitChange, i);

    buttons[i]
    .begin(buttonPins[i])
    .onPress(buttonBits[i], buttonBits[i].EVT_TOGGLE);
  }
}

/**
   Digital switch functions
*/

void onDigitalSwitch(int idx, int v, int up) {
  Serial.print("DSW:");
  Serial.println(idx);
}

void onCorrectDigitalSwitchesCombi() {
  if (progState.solvedDigitalSwitches) {
    return;
  }

  Serial.println("DSW:V");
  progState.solvedDigitalSwitches = true;
}

bool isCorrectDigitalSwitchesCombi() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    if (digitalSwitches[i].state() != Atm_button::PRESSED) {
      return false;
    }
  }

  return true;
}

void initDigitalSwitches() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    digitalSwitches[i]
    .begin(digitalSwitchPins[i])
    .onPress(onDigitalSwitch, i);
  }
}

/**
   Potentiometer functions
*/

void onCorrectPotsCombi() {
  if (progState.solvedPots) {
    return;
  }

  Serial.println("P:V");
  progState.solvedPots = true;
}

bool checkPotsCombination() {
  bool isValid;

  for (int i = 0; i < NUM_POT_PATTERNS; i++) {
    isValid = true;

    for (int j = 0; j < NUM_POTS; j++) {
      if (pots[j].state() != potPatterns[i][j]) {
        isValid = false;
      }
    }

    if (isValid) {
      return true;
    }
  }

  return false;
}

bool isCorrectPotsCombi() {
  if (progState.solvedPots) {
    return true;
  }

  for (int i = 0; i < NUM_CHECKS; i++) {
    if (checkPotsCombination() == false) {
      return false;
    }

    delay(DELAY_CHECK_MS);
  }

  return checkPotsCombination();
}

void onPotChange(int idx, int v, int up) {
  Serial.print("P:");
  Serial.print(idx);
  Serial.print(":");
  Serial.println(v);
}

void initPots() {
  for (int i = 0; i < NUM_POTS; i++) {
    pots[i]
    .begin(potPins[i])
    .range(POTS_RANGE_LO, POTS_RANGE_HI)
    .onChange(onPotChange, i);
  }
}

void checkSolutions() {
  if (isCorrectDigitalSwitchesCombi()) {
    onCorrectDigitalSwitchesCombi();
  }

  if (isCorrectButtonsCombi()) {
    onCorrectButtonsCombi();
  }

  if (isCorrectPotsCombi()) {
    onCorrectPotsCombi();
  }
}

void setup() {
  Serial.begin(9600);

  initDigitalSwitches();
  initButtons();
  initPots();

  Serial.println(">> Serum");
}

void loop() {
  automaton.run();
  checkSolutions();
}
