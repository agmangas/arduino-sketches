#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>
#include <Keypad.h>
#include <limits.h>

/**
 * LED strips.
 */

const uint8_t LED_PIN_PAD = 11;
const uint8_t LED_PIN = 12;

const uint16_t NUM_LEDS = 60;
const uint16_t NUM_LEDS_PAD = 1;

const int LED_BRIGHTNESS = 180;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(
    NUM_LEDS,
    LED_PIN,
    NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel pixelPad = Adafruit_NeoPixel(
    NUM_LEDS_PAD,
    LED_PIN_PAD,
    NEO_RGB + NEO_KHZ800);

const uint32_t COLOR_IDLE = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(226, 148, 58));

const uint32_t COLOR_OK = Adafruit_NeoPixel::Color(0, 255, 0);
const uint32_t COLOR_ERR = Adafruit_NeoPixel::Color(255, 0, 0);

/**
 * Keypad.
 */

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 2, 3, 4, 5 };
byte colPins[COLS] = { 6, 7, 8, 9 };

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int SOLUTION_SIZE = 4;

char solutionKeys[SOLUTION_SIZE] = {
    '1',
    '9',
    '1',
    '7  ',
};

/**
 * Inputs buffer.
 */

CircularBuffer<char, SOLUTION_SIZE> keyBuffer;

/**
 * LED functions.
 */

void initLeds()
{
    pixelStrip.begin();
    pixelStrip.setBrightness(LED_BRIGHTNESS);
    pixelStrip.clear();
    pixelStrip.show();

    pixelPad.begin();
    pixelPad.setBrightness(LED_BRIGHTNESS);
    pixelPad.clear();
    pixelPad.show();
}

void showStartupEffect()
{
    const int delayMs = 1500;
    const uint32_t color = Adafruit_NeoPixel::Color(0, 255, 0);

    pixelStrip.fill(color);
    pixelStrip.show();
    delay(delayMs);
    pixelStrip.clear();
    pixelStrip.show();

    pixelPad.fill(color);
    pixelPad.show();
    delay(delayMs);
    pixelPad.clear();
    pixelPad.show();

    pixelPad.fill(COLOR_IDLE);
    pixelPad.show();
}

void showPadPressEffect()
{
    const int delayMs = 50;
    const int numLoops = 2;

    for (int i = 0; i < numLoops; i++) {
        pixelPad.clear();
        pixelPad.show();
        delay(delayMs);
        pixelPad.fill(COLOR_IDLE);
        pixelPad.show();
        delay(delayMs);
    }
}

void showPadErrorEffect()
{
    const int delayMs = 100;
    const int numLoops = 10;

    for (int i = 0; i < numLoops; i++) {
        pixelPad.clear();
        pixelPad.show();
        delay(delayMs);
        pixelPad.fill(COLOR_ERR);
        pixelPad.show();
        delay(delayMs);
    }

    pixelPad.fill(COLOR_IDLE);
    pixelPad.show();
}

void showFinishEffect(int numLoops)
{
    const int delayMs = 300;

    numLoops = numLoops > 0 ? numLoops : INT_MAX;

    const uint32_t colorOne = Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(255, 127, 0));

    const uint32_t colorTwo = Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(160, 32, 240));

    const uint32_t colorThree = Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(50, 205, 50));

    while (true) {
        pixelStrip.fill(colorOne);
        pixelStrip.show();
        delay(delayMs);
        pixelStrip.fill(colorTwo);
        pixelStrip.show();
        delay(delayMs);
        pixelStrip.fill(colorThree);
        pixelStrip.show();
        delay(delayMs);
    }
}

/**
 * Keypad and buffer functions.
 */

void updateKeyBuffer()
{
    char key = kpd.getKey();

    if (key != NO_KEY) {
        Serial.print(F("Key: "));
        Serial.println(key);
        keyBuffer.push(key);
        showPadPressEffect();
    }
}

bool isCodeInBuffer(char* code)
{
    if (keyBuffer.size() < SOLUTION_SIZE) {
        return false;
    }

    for (int i = 0; i < keyBuffer.size(); i++) {
        if (keyBuffer[i] != code[i]) {
            return false;
        }
    }

    return true;
}

void keypadLoop()
{
    updateKeyBuffer();

    if (keyBuffer.available() > 0) {
        return;
    }

    Serial.println(F("Buffer is full"));

    if (isCodeInBuffer(solutionKeys)) {
        pixelPad.fill(COLOR_OK);
        pixelPad.show();
        showFinishEffect(0);
    } else {
        showPadErrorEffect();
    }

    keyBuffer.clear();
};

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initLeds();

    Serial.println(F(">> Starting Hydra-Pad program"));

    showStartupEffect();
}

void loop()
{
    keypadLoop();
}
