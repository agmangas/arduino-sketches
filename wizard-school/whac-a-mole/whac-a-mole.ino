#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Piezo knock sensors.
 */

const int KNOCK_PIN = A5;
const int KNOCK_SAMPLERATE = 50;
const int KNOCK_RANGE_MIN = 0;
const int KNOCK_RANGE_MAX = 100;
const int KNOCK_THRESHOLD = 10;

Atm_analog knockAnalog;
Atm_controller knockController;

/**
   LED strips.
*/

const int LED_BRIGHTNESS = 50;
const int LED_PIN = 3;
const int LED_NUM = 1;

const uint32_t LED_COLOR = Adafruit_NeoPixel::Color(100, 255, 0);

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
   LED functions.
*/

void initLedStrip()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_BRIGHTNESS);
    ledStrip.show();
    clearLeds();
}

void clearLeds()
{
    ledStrip.clear();
    ledStrip.show();
}

void blinkLed()
{
    ledStrip.setPixelColor(0, LED_COLOR);
    ledStrip.show();
    delay(150);
    ledStrip.setPixelColor(0, 0);
    ledStrip.show();
}

/**
 * Knock sensor functions.
 */

void onKnock(int idx, int v, int up)
{
    Serial.println(F("Knock"));
    blinkLed();
}

void initKnockSensor()
{
    knockAnalog
        .begin(KNOCK_PIN, KNOCK_SAMPLERATE)
        .range(KNOCK_RANGE_MIN, KNOCK_RANGE_MAX);

    knockController
        .begin()
        .IF(knockAnalog, '>', KNOCK_THRESHOLD)
        .onChange(true, onKnock);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initKnockSensor();
    initLedStrip();

    Serial.println(F(">> Starting whac-a-mole program"));
}

void loop()
{
    automaton.run();
}