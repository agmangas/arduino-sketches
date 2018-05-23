#include <Adafruit_NeoPixel.h>

// analogRead() range
const int MAX_ANALOG_READ = 1023;

// Total number of inputs
const int NUM_INPUTS = 4;

// Potentiometer pins
byte potPins[NUM_INPUTS] = {1, 2, 3, 4};

// Potentiometer discrete levels
const int POT_LEVELS = 30;

// Relay pin
const byte RELAY_PIN = 10;

// Startup wait
const int STARTUP_WAIT_MS = 3000;

// Wait interval (ms) between each LED activation just before the relay is activated
const int OPEN_LED_STEP_WAIT_MS = 300;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 30;
const uint8_t NEOPIXEL_PIN_1 = 4;
const uint8_t NEOPIXEL_PIN_2 = 5;
const uint8_t NEOPIXEL_PIN_3 = 6;
const uint8_t NEOPIXEL_PIN_4 = 7;
const uint8_t NEOPIXEL_PIN_SOLUTION = 8;

// Solution key
const int SOLUTION_INPUT_1 = 25;
const int SOLUTION_INPUT_2 = 10;
const int SOLUTION_INPUT_3 = 10;
const int SOLUTION_INPUT_4 = 25;

// Audio tracks pins
const int PIN_AUDIO_TRACK_END = 12;

// Initialize the NeoPixel instances
Adafruit_NeoPixel pixelStrip1 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip2 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip3 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip4 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStripSolution = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_SOLUTION, NEO_GRB + NEO_KHZ800);

// Current input levels
int inputLevel1 = 0;
int inputLevel2 = 0;
int inputLevel3 = 0;
int inputLevel4 = 0;

/**
   Applies potentiometer inputs to the LED strips.
*/
void readInputAndUpdate(int inputIdx, Adafruit_NeoPixel &pixelStrip, int &currentInputLevel) {
  int potVal = analogRead(potPins[inputIdx]);
  float relativePotVal = potVal / (float) MAX_ANALOG_READ;
  int inputLevel = ceil(relativePotVal * POT_LEVELS);

  if (inputLevel == 0) inputLevel = 1;

  int pixelsPerLevel = ceil(NEOPIXEL_NUM / (float) POT_LEVELS);
  int totalPixelsOn = inputLevel * pixelsPerLevel;

  if (totalPixelsOn > NEOPIXEL_NUM) totalPixelsOn = NEOPIXEL_NUM;

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    if (i < totalPixelsOn) {
      pixelStrip.setPixelColor(i, random(50, 250), 0, 0);
    } else {
      pixelStrip.setPixelColor(i, 0, 0, 0);
    }
  }

  pixelStrip.show();

  currentInputLevel = inputLevel;
}

/**
   Returns true if the current input levels are the correct solution.
*/
bool isValidSolution() {
  Serial.print("Input levels: ");
  Serial.print(inputLevel1);
  Serial.print(" / ");
  Serial.print(inputLevel2);
  Serial.print(" / ");
  Serial.print(inputLevel3);
  Serial.print(" / ");
  Serial.print(inputLevel4);
  Serial.println();
  Serial.flush();

  bool ok1 = inputLevel1 == SOLUTION_INPUT_1;
  bool ok2 = inputLevel2 == SOLUTION_INPUT_2;
  bool ok3 = inputLevel3 == SOLUTION_INPUT_3;
  bool ok4 = inputLevel4 == SOLUTION_INPUT_4;

  return ok1 && ok2 && ok3 && ok4;
}

/**
   Turn the final LED strip on and open the relay.
*/
void openLockAndWait() {
  Serial.println("## Turning final LED strip on");
  Serial.flush();

  playTrack(PIN_AUDIO_TRACK_END);

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStripSolution.setPixelColor(i, 0, 0, 0);
  }

  pixelStripSolution.show();

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStripSolution.setPixelColor(i, 255, 0, 0);
    pixelStripSolution.show();

    delay(OPEN_LED_STEP_WAIT_MS);
  }

  Serial.println("## Opening relay");
  Serial.flush();

  digitalWrite(RELAY_PIN, HIGH);

  while (true) {
    randomizeAllPixels();
    delay(OPEN_LED_STEP_WAIT_MS);
  }
}

/**
   Randomizes all strips pixels.
*/
void randomizeAllPixels() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip1.setPixelColor(i, random(100, 220), 0, 0);
    pixelStrip2.setPixelColor(i, random(100, 220), 0, 0);
    pixelStrip3.setPixelColor(i, random(100, 220), 0, 0);
    pixelStrip4.setPixelColor(i, random(100, 220), 0, 0);
    pixelStripSolution.setPixelColor(i, random(100, 220), 0, 0);
  }

  pixelStrip1.show();
  pixelStrip2.show();
  pixelStrip3.show();
  pixelStrip4.show();
  pixelStripSolution.show();
}

/**
   Plays the audio track connected to the given pin.
*/
void playTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_AUDIO_TRACK_END, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_END, HIGH);

  pixelStrip1.begin();
  pixelStrip1.setBrightness(100);
  pixelStrip1.show();

  pixelStrip2.begin();
  pixelStrip2.setBrightness(100);
  pixelStrip2.show();

  pixelStrip3.begin();
  pixelStrip3.setBrightness(100);
  pixelStrip3.show();

  pixelStrip4.begin();
  pixelStrip4.setBrightness(100);
  pixelStrip4.show();

  pixelStripSolution.begin();
  pixelStripSolution.setBrightness(200);
  pixelStripSolution.show();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.println(">> Starting Reanimathor Phase 01 program");

  delay(STARTUP_WAIT_MS);
}

void loop() {
  readInputAndUpdate(0, pixelStrip1, inputLevel1);
  readInputAndUpdate(1, pixelStrip2, inputLevel2);
  readInputAndUpdate(2, pixelStrip3, inputLevel3);
  readInputAndUpdate(3, pixelStrip4, inputLevel4);

  if (isValidSolution()) {
    openLockAndWait();
  }

  delay(50);
}
