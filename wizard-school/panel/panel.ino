#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Digital switch.
 */

const int DSWITCH_NUM = 8;

const int DSWITCH_PINS[DSWITCH_NUM] = {
    2, 3, 4, 5, 6, 7, 8, 9};

Atm_button dswitchButton[DSWITCH_NUM];

/**
 * Proximity sensors.
 */

const int PROX_SENSORS_LEFT_NUM = 3;

const int PROX_SENSORS_LEFT_PINS[PROX_SENSORS_LEFT_NUM] = {
    A1, A2, A3};

const int PROX_SENSORS_RIGHT_PIN = A0;

Atm_button proxSensorsLeft[PROX_SENSORS_LEFT_NUM];
Atm_button proxSensorsRight;

/**
 * Throughhole LEDs.
 */

const int LED_THROUGH_BRIGHTNESS = 150;
const int LED_THROUGH_NUM = DSWITCH_NUM;
const int LED_THROUGH_PIN = 11;

Adafruit_NeoPixel ledThrough = Adafruit_NeoPixel(
    LED_THROUGH_NUM,
    LED_THROUGH_PIN,
    NEO_RGB + NEO_KHZ800);

const int LED_THROUGH_COLOR_NUM = 4;

const uint32_t LED_THROUGH_COLORS[LED_THROUGH_COLOR_NUM] = {
    Adafruit_NeoPixel::Color(255, 0, 0),
    Adafruit_NeoPixel::Color(0, 255, 0),
    Adafruit_NeoPixel::Color(0, 0, 255),
    Adafruit_NeoPixel::Color(128, 0, 128)};

/**
 * Strip LEDs.
 */

const int LED_STRIP_BRIGHTNESS = 150;
const int LED_STRIP_NUM = 12;
const int LED_STRIP_PIN = 12;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(
    LED_STRIP_NUM,
    LED_STRIP_PIN,
    NEO_GRB + NEO_KHZ800);

const int LED_STRIP_SUBSTRIP_NUM = PROX_SENSORS_LEFT_NUM;

const int LED_STRIP_SUBSTRIP_LIMITS[LED_STRIP_SUBSTRIP_NUM][2] = {
    {0, 3},
    {4, 7},
    {8, 11}};

const int LED_STRIP_COLOR_NUM = 5;

const uint32_t LED_STRIP_COLORS[LED_STRIP_COLOR_NUM] = {
    Adafruit_NeoPixel::Color(255, 0, 0),
    Adafruit_NeoPixel::Color(0, 255, 0),
    Adafruit_NeoPixel::Color(0, 0, 255),
    Adafruit_NeoPixel::Color(128, 0, 128),
    Adafruit_NeoPixel::Color(128, 128, 0)};

/**
 * LED refresh timer.
 */

Atm_timer ledRefreshTimer;

const int LED_REFRESH_TIMER_MS = 200;

/**
 * Relays.
 */

const int RELAY_PIN_ONE = A5;
const int RELAY_PIN_TWO = A6;

/**
 * Program state.
 */

int currThroughColorIdx[LED_THROUGH_NUM];
int currSubstripColorIdx[LED_STRIP_SUBSTRIP_NUM];

typedef struct programState
{
    int *currThroughColorIdx;
    int currThroughIdx;
    uint8_t blinkCounter;
    int *currSubstripColorIdx;
} ProgramState;

ProgramState progState = {
    .currThroughColorIdx = currThroughColorIdx,
    .currThroughIdx = 0,
    .blinkCounter = 0,
    .currSubstripColorIdx = currSubstripColorIdx};

void initState()
{
    for (int i = 0; i < LED_THROUGH_NUM; i++)
    {
        progState.currThroughColorIdx[i] = random(0, LED_THROUGH_COLOR_NUM);
    }

    for (int i = 0; i < LED_STRIP_SUBSTRIP_NUM; i++)
    {
        progState.currSubstripColorIdx[i] = 0;
    }
}

/**
 * Digital switch functions.
 */

void onDswitch(int idx, int v, int up)
{
    Serial.print(F("Dswitch :: "));
    Serial.println(idx);

    progState.currThroughIdx = idx;
}

void initDswitch()
{
    for (int i = 0; i < DSWITCH_NUM; i++)
    {
        dswitchButton[i]
            .begin(DSWITCH_PINS[i])
            .onPress(onDswitch, i);
    }
}

/**
 * Proximity sensor functions.
 */

void onLeftProxSensor(int idx, int v, int up)
{
    Serial.print(F("Left prox sensor :: "));
    Serial.println(idx);

    progState.currSubstripColorIdx[idx]++;

    progState.currSubstripColorIdx[idx] =
        progState.currSubstripColorIdx[idx] % LED_STRIP_COLOR_NUM;
}

void onRightProxSensor(int idx, int v, int up)
{
    Serial.println(F("Right prox sensor"));

    int currIdx = progState.currThroughIdx;

    progState.currThroughColorIdx[currIdx]++;

    progState.currThroughColorIdx[currIdx] =
        progState.currThroughColorIdx[currIdx] % LED_THROUGH_COLOR_NUM;
}

void initProximitySensors()
{
    for (int i = 0; i < PROX_SENSORS_LEFT_NUM; i++)
    {
        proxSensorsLeft[i]
            .begin(PROX_SENSORS_LEFT_PINS[i])
            .onPress(onLeftProxSensor, i);
    }

    proxSensorsRight
        .begin(PROX_SENSORS_RIGHT_PIN)
        .onPress(onRightProxSensor);
}

/**
 * LED throughhole functions.
 */

void initLedThrough()
{
    ledThrough.begin();
    ledThrough.setBrightness(LED_THROUGH_BRIGHTNESS);
    ledThrough.clear();
    ledThrough.show();
}

void refreshLedThrough()
{
    uint32_t color;
    bool isBlink;

    ledThrough.clear();

    for (int i = 0; i < LED_THROUGH_NUM; i++)
    {
        isBlink = i == progState.currThroughIdx &&
                  progState.blinkCounter % 2 == 0;

        color = isBlink ? 0 : LED_THROUGH_COLORS[progState.currThroughColorIdx[i]];

        ledThrough.setPixelColor(i, color);
    }

    ledThrough.show();

    progState.blinkCounter++;
}

/**
 * LED strip functions.
 */

void initLedStrip()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_THROUGH_BRIGHTNESS);
    ledStrip.clear();
    ledStrip.show();
}

void refreshLedStrip()
{
    ledStrip.clear();

    for (int i = 0; i < LED_STRIP_SUBSTRIP_NUM; i++)
    {
        uint32_t color = LED_STRIP_COLORS[progState.currSubstripColorIdx[i]];

        ledStrip.setPixelColor(
            LED_STRIP_SUBSTRIP_LIMITS[i][0],
            color);

        ledStrip.setPixelColor(
            LED_STRIP_SUBSTRIP_LIMITS[i][1],
            color);
    }

    ledStrip.show();
}

/**
 * LED refresh timer functions.
 */

void onLedTimer(int idx, int v, int up)
{
    refreshLedThrough();
    refreshLedStrip();
}

void initLedTimer()
{
    ledRefreshTimer
        .begin(LED_REFRESH_TIMER_MS)
        .repeat(-1)
        .onTimer(onLedTimer)
        .start();
}

/**
 * Relay functions.
 */

void lockRelay(int pin)
{
    digitalWrite(pin, LOW);
}

void openRelay(int pin)
{
    digitalWrite(pin, HIGH);
}

void initRelay(int pin)
{
    pinMode(pin, OUTPUT);
    lockRelay(pin);
}

void initRelays()
{
    initRelay(RELAY_PIN_ONE);
    initRelay(RELAY_PIN_TWO);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initProximitySensors();
    initDswitch();
    initLedThrough();
    initLedStrip();
    initRelays();
    initLedTimer();

    Serial.println(F(">> Starting panel program"));
}

void loop()
{
    automaton.run();
}