#include <Adafruit_NeoPixel.h>

// analogRead() range
const int MAX_ANALOG_READ = 1023;

// Total number of inputs
const int NUM_INPUTS = 4;

// Potentiometer pins
byte potPins[NUM_INPUTS] = {1, 2, 3, 4};

// Potentiometer discrete levels
const int POT_LEVELS = 30;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 30;
const uint8_t NEOPIXEL_PIN_1 = 4;
const uint8_t NEOPIXEL_PIN_2 = 5;
const uint8_t NEOPIXEL_PIN_3 = 6;
const uint8_t NEOPIXEL_PIN_4 = 7;

// Initialize the NeoPixel instance
Adafruit_NeoPixel pixelStrip1 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip2 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip3 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStrip4 = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN_4, NEO_GRB + NEO_KHZ800);

/**
   Applies potentiometer inputs to the LED strips.
*/
void applyPotValuesToLeds(int inputIdx, Adafruit_NeoPixel *thePixelStrip) {
  int potVal = analogRead(potPins[inputIdx]);
  float relativePotVal = potVal / (float) MAX_ANALOG_READ;
  int inputLevel = ceil(relativePotVal * POT_LEVELS);

  if (inputLevel == 0) inputLevel = 1;

  Serial.print("inputLevel :: ");
  Serial.print(inputIdx);
  Serial.print(" :: ");
  Serial.println(inputLevel);
  Serial.flush();

  int pixelsPerLevel = ceil(NEOPIXEL_NUM / (float) POT_LEVELS);
  int totalPixelsOn = inputLevel * pixelsPerLevel;

  if (totalPixelsOn > NEOPIXEL_NUM) totalPixelsOn = NEOPIXEL_NUM;

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    if (i < totalPixelsOn) {
      thePixelStrip->setPixelColor(i, random(50, 250), 0, 0);
    } else {
      thePixelStrip->setPixelColor(i, 0, 0, 0);
    }
  }

  thePixelStrip->show();
}

void setup() {
  Serial.begin(9600);

  Serial.println("Starting RFID electronic lock program");
  Serial.flush();

  pixelStrip1.begin();
  pixelStrip1.setBrightness(220);
  pixelStrip1.show();

  pixelStrip2.begin();
  pixelStrip2.setBrightness(220);
  pixelStrip2.show();

  pixelStrip3.begin();
  pixelStrip3.setBrightness(220);
  pixelStrip3.show();

  pixelStrip4.begin();
  pixelStrip4.setBrightness(220);
  pixelStrip4.show();
}

void loop() {
  applyPotValuesToLeds(0, &pixelStrip1);
  applyPotValuesToLeds(1, &pixelStrip2);
  applyPotValuesToLeds(2, &pixelStrip3);
  applyPotValuesToLeds(3, &pixelStrip4);

  delay(100);
}
