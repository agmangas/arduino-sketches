#include <WS2812FX.h>

/**
 * LED strips.
 */

const int LED_BRIGHTNESS = 150;
// From 10 (very fast) to 5000 (very slow)
const int LED_SPEED = 200;
const int LED_NUM = 10;
const int LED_PIN = 10;

WS2812FX ws2812fx = WS2812FX(
    LED_NUM,
    LED_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * LED functions.
 */

void initLed()
{
    ws2812fx.init();
    ws2812fx.setBrightness(LED_BRIGHTNESS);
    ws2812fx.setSpeed(LED_SPEED);
    ws2812fx.setColor(RED);
    ws2812fx.setMode(FX_MODE_FIRE_FLICKER);
    ws2812fx.start();
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initLed();

    Serial.println(F(">> Starting torch program"));
}

void loop()
{
    ws2812fx.service();
}