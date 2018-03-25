#include <Adafruit_NeoPixel.h>

// analogRead() range
const int MAX_ANALOG_READ = 1023;

// Total number of inputs
const int NUM_INPUTS = 1;

// Potentiometer pins
byte potPins[NUM_INPUTS] = {1};

// Potentiometer discrete levels
const int POT_LEVELS = 30;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 30;
const uint8_t NEOPIXEL_PIN = 3;

// Initialize the NeoPixel instance
Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Some NeoPixel colors
uint32_t colorActive = pixelStrip.Color(255, 0, 0);

/**
   Turns all the LEDs off.
*/
void turnOffPixels() {
  Serial.println("Turning all the LEDs off");
  Serial.flush();

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

/**
   Turns all the LEDs on.
*/
void turnOnPixels() {
  Serial.println("Turning all the LEDs on");
  Serial.flush();

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, colorActive);
  }

  pixelStrip.show();
}

/**
   Applies potentiometer inputs to the LED strips.
*/
void applyPotValuesToLeds(int inputIdx) {
  int potVal = analogRead(potPins[inputIdx]);
  float relativePotVal = potVal / (float) MAX_ANALOG_READ;
  int inputLevel = ceil(relativePotVal * POT_LEVELS);

  if (inputLevel == 0) inputLevel = 1;

  Serial.print("inputLevel: ");
  Serial.println(inputLevel);
  Serial.flush();

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
}

void setup() {
  Serial.begin(9600);

  Serial.println("Starting RFID electronic lock program");
  Serial.flush();

  pixelStrip.begin();
  pixelStrip.setBrightness(50);
  pixelStrip.show();
}

void loop() {
  applyPotValuesToLeds(0);
  delay(100);
}
