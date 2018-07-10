#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

typedef struct programState {
  bool solvedDigitalSwitches;
  bool solvedButtons;
  bool solvedPots;
} ProgramState;

/**
   Global
*/

const int DELAY_CHECK_MS = 25;
const int NUM_CHECKS = 20;

/**
   Relay
*/

const int PIN_RELAY = A5;

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
  false, false, false, true, true, false, true
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

const int NUM_POT_PATTERNS = 2;

byte potPatterns[NUM_POT_PATTERNS][NUM_POTS] = {
  {1, 3, 0, 2},
  {1, 4, 0, 2}
};

/**
   LED
*/

const uint16_t NEOPIXEL_NUM = 21;
const uint8_t NEOPIXEL_PIN = 13;

const byte buttonLeds[NUM_BUTTONS] = {
  0, 2, 4, 6, 8, 10, 12
};

const byte DSWITCH_SIGNAL_LED_START = 15;
const byte DSWITCH_SIGNAL_LED_END = 16;

const byte BUTTONS_SIGNAL_LED_START = 17;
const byte BUTTONS_SIGNAL_LED_END = 18;

const byte POTS_SIGNAL_LED_START = 19;
const byte POTS_SIGNAL_LED_END = 20;

const uint32_t COLOR_BUTTONS = Adafruit_NeoPixel::Color(0, 255, 0);
const uint32_t COLOR_SIGNALS = Adafruit_NeoPixel::Color(255, 0, 0);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

/**
   Program state
*/

ProgramState progState = {
  .solvedDigitalSwitches = false,
  .solvedButtons = false,
  .solvedPots = false
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

  Serial.println("Btn:OK");
  progState.solvedButtons = true;
}

bool checkButtonsCombination() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if ((buttonBits[i].state() == 0 && buttonsPattern[i] == true) ||
        (buttonBits[i].state() == 1 && buttonsPattern[i] == false)) {
      return false;
    }
  }

  return true;
}

bool isCorrectButtonsCombi() {
  for (int i = 0; i < NUM_CHECKS; i++) {
    if (checkButtonsCombination() == false) {
      return false;
    }

    delay(DELAY_CHECK_MS);
  }

  return checkButtonsCombination();
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

  Serial.println("DSW:OK");
  progState.solvedDigitalSwitches = true;
}

bool checkDigitalSwitchesCombination() {
  for (int i = 0; i < NUM_DIGITAL_SWITCH; i++) {
    if (digitalSwitches[i].state() != Atm_button::PRESSED) {
      return false;
    }
  }

  return true;
}

bool isCorrectDigitalSwitchesCombi() {
  for (int i = 0; i < NUM_CHECKS; i++) {
    if (checkDigitalSwitchesCombination() == false) {
      return false;
    }

    delay(DELAY_CHECK_MS);
  }

  return checkDigitalSwitchesCombination();
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

  Serial.println("P:OK");
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

/**
   LED functions
*/

void initStrip() {
  pixelStrip.begin();
  pixelStrip.setBrightness(100);
  pixelStrip.clear();
  pixelStrip.show();
}

void showButtonsSignalStrip() {
  for (int i = BUTTONS_SIGNAL_LED_START; i <= BUTTONS_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }

  pixelStrip.show();
}

void showDswitchSignalStrip() {
  for (int i = DSWITCH_SIGNAL_LED_START; i <= DSWITCH_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }

  pixelStrip.show();
}

void showPotsSignalStrip() {
  for (int i = POTS_SIGNAL_LED_START; i <= POTS_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }

  pixelStrip.show();
}

void showButtonsStrip() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonBits[i].state() == 0) {
      pixelStrip.setPixelColor(buttonLeds[i], 0, 0, 0);
    } else {
      pixelStrip.setPixelColor(buttonLeds[i], COLOR_BUTTONS);
    }
  }

  pixelStrip.show();
}

void showStrip() {
  showButtonsStrip();

  if (progState.solvedPots) {
    showPotsSignalStrip();
  }

  if (progState.solvedDigitalSwitches) {
    showDswitchSignalStrip();
  }

  if (progState.solvedButtons) {
    showButtonsStrip();
  }
}

/**
   Relay functions
*/

void lockRelay() {
  digitalWrite(PIN_RELAY, LOW);
}

void openRelay() {
  if (digitalRead(PIN_RELAY) == LOW) {
    Serial.println("Relay:Open");
    digitalWrite(PIN_RELAY, HIGH);
  }
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

/**
   Utility functions and entrypoint
*/

void checkSolutionState() {
  if (!progState.solvedDigitalSwitches &&
      isCorrectDigitalSwitchesCombi()) {
    onCorrectDigitalSwitchesCombi();
  }

  if (!progState.solvedButtons &&
      isCorrectButtonsCombi()) {
    onCorrectButtonsCombi();
  }

  if (!progState.solvedPots &&
      isCorrectPotsCombi()) {
    onCorrectPotsCombi();
  }

  if (progState.solvedDigitalSwitches &&
      progState.solvedButtons &&
      progState.solvedPots) {
    openRelay();
  }
}

void setup() {
  Serial.begin(9600);

  initDigitalSwitches();
  initButtons();
  initPots();
  initStrip();
  initRelay();

  Serial.println(">> Serum");
}

void loop() {
  automaton.run();
  checkSolutionState();
  showStrip();
}
