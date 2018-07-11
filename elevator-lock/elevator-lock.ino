#include <Keypad.h>
#include <CircularBuffer.h>

/**
   Program state
*/

typedef struct programState {
  bool keypadSolved;
} ProgramState;

ProgramState progState = {
  .keypadSolved = false
};

/**
   Relay
*/

const int PIN_RELAY = A0;

/**
   LEDs
*/

const int LED_A = 2;
const int LED_B = 3;

/**
   Keypad init
*/

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = { 4, 5, 6, 7 };
byte colPins[COLS] = { 8, 9, 10, 11 };

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int SOLUTION_SIZE = 4;

char solutionKeys[SOLUTION_SIZE] = {'*', '8',  '6', 'A',};

/**
   Inputs buffer
*/

CircularBuffer<char, SOLUTION_SIZE> keyBuffer;

/**
   Keypad and buffer functions
*/

void updateKeyBuffer() {
  char key = kpd.getKey();

  if (key != NO_KEY) {
    Serial.print("Key:");
    Serial.println(key);
    keyBuffer.push(key);
  }
}

bool isSolutionInBuffer() {
  if (keyBuffer.size() < SOLUTION_SIZE) {
    return false;
  }

  for (int i = 0; i < keyBuffer.size(); i++) {
    if (keyBuffer[i] != solutionKeys[i]) {
      return false;
    }
  }

  return true;
}

void onKeypadSolution() {
  openRelay();
  ledOff(LED_A);
  ledOn(LED_B);
}

void runKeypad() {
  updateKeyBuffer();

  if (isSolutionInBuffer() && !progState.keypadSolved) {
    Serial.println("Keypad:OK");
    progState.keypadSolved = true;
    onKeypadSolution();
  }
};

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
   LED functions
*/

void initLeds() {
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  ledOff(LED_A);
  ledOff(LED_B);
}

void toggleLed(int ledPin) {
  digitalWrite(ledPin, !digitalRead(ledPin));
}

void ledOn(int ledPin) {
  digitalWrite(ledPin, HIGH);
}


void ledOff(int ledPin) {
  digitalWrite(ledPin, LOW);
}

/**
   Entrypoint
*/

void setup() {
  Serial.begin(9600);

  initRelay();
  initLeds();
  ledOn(LED_A);

  Serial.println(">> Elevator Lock program");
}

void loop() {
  runKeypad();
}
