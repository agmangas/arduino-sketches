#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
   Program state
*/

typedef struct programState {
  int counterDigitalSwitches;
  int counterButtons;
  int counterPots;
} ProgramState;

ProgramState progState = {
  .counterDigitalSwitches = 0,
  .counterButtons = 0,
  .counterPots = 0
};

const int COUNTER_THRESHOLD = 100;
const uint16_t DELAY_LOOP_MS = 10;

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

const uint32_t COLOR_BUTTONS = Adafruit_NeoPixel::Color(0, 0, 255);
const uint32_t COLOR_SIGNALS = Adafruit_NeoPixel::Color(0, 255, 0);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

/**
   Button functions
*/

void onBitChange(int idx, int v, int up) {
  Serial.print("Bit:");
  Serial.print(idx);
  Serial.print(":");
  Serial.println(buttonBits[idx].state());
}

bool isCorrectButtonsCombi() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if ((buttonBits[i].state() == 0 && buttonsPattern[i] == true) ||
        (buttonBits[i].state() == 1 && buttonsPattern[i] == false)) {
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


void onPotChange(int idx, int v, int up) {
  Serial.print("P:");
  Serial.print(idx);
  Serial.print(":");
  Serial.println(v);
}

bool isCorrectPotsCombi() {
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

void updateButtonsSignalStrip() {
  for (int i = BUTTONS_SIGNAL_LED_START; i <= BUTTONS_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }
}

void updateDswitchSignalStrip() {
  for (int i = DSWITCH_SIGNAL_LED_START; i <= DSWITCH_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }
}

void updatePotsSignalStrip() {
  for (int i = POTS_SIGNAL_LED_START; i <= POTS_SIGNAL_LED_END; i++) {
    pixelStrip.setPixelColor(i, COLOR_SIGNALS);
  }
}

void updateButtonsStrip() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonBits[i].state() == 0) {
      pixelStrip.setPixelColor(buttonLeds[i], 0, 0, 0);
    } else {
      pixelStrip.setPixelColor(buttonLeds[i], COLOR_BUTTONS);
    }
  }
}

/**
   Relay functions
*/

void lockRelay() {
  if (digitalRead(PIN_RELAY) == HIGH) {
    Serial.println("Relay:Lock");
  }

  digitalWrite(PIN_RELAY, LOW);
}

void openRelay() {
  if (digitalRead(PIN_RELAY) == LOW) {
    Serial.println("Relay:Open");
  }

  digitalWrite(PIN_RELAY, HIGH);
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

/**
   Utility functions and entrypoint
*/

void updateSolutionState() {
  bool okDswitch = isCorrectDigitalSwitchesCombi();
  bool okButtons = isCorrectButtonsCombi();
  bool okPots = isCorrectPotsCombi();

  if (okDswitch && progState.counterDigitalSwitches < COUNTER_THRESHOLD) {
    if (progState.counterDigitalSwitches == (COUNTER_THRESHOLD - 1)) {
      Serial.println("Dsw:OK");
    }

    progState.counterDigitalSwitches++;
  } else if (!okDswitch) {
    progState.counterDigitalSwitches = 0;
  }

  if (okButtons && progState.counterButtons < COUNTER_THRESHOLD) {
    if (progState.counterButtons == (COUNTER_THRESHOLD - 1)) {
      Serial.println("Btn:OK");
    }

    progState.counterButtons++;
  } else if (!okButtons) {
    progState.counterButtons = 0;
  }

  if (okPots && progState.counterPots < COUNTER_THRESHOLD) {
    if (progState.counterPots == (COUNTER_THRESHOLD - 1)) {
      Serial.println("Pot:OK");
    }

    progState.counterPots++;
  } else if (!okPots) {
    progState.counterPots = 0;
  }

  bool activeDswitch = progState.counterDigitalSwitches >= COUNTER_THRESHOLD;
  bool activeButtons = progState.counterButtons >= COUNTER_THRESHOLD;
  bool activePots = progState.counterPots >= COUNTER_THRESHOLD;

  if (activeDswitch) {
    updateDswitchSignalStrip();
  }

  if (activeButtons) {
    updateButtonsSignalStrip();
  }

  if (activePots) {
    updatePotsSignalStrip();
  }

  if (activeDswitch && activeButtons && activePots) {
    openRelay();
  } else {
    lockRelay();
  }
}

void setup() {
  Serial.begin(9600);

  initDigitalSwitches();
  initButtons();
  initPots();
  initStrip();
  initRelay();

  Serial.println(">> Life Serum program");
}

void loop() {
  automaton.run();
  pixelStrip.clear();
  updateButtonsStrip();
  updateSolutionState();
  pixelStrip.show();
  delay(DELAY_LOOP_MS);
}
