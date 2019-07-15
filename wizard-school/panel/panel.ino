#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Color gamma correction.
 */

const uint8_t PROGMEM gamma8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

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

const int LED_THROUGH_KEY[LED_THROUGH_NUM] = {
    2, 0, 3, 1, 3, 0, 1, 2};

Atm_controller ledThroughController;

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

const int LED_STRIP_COLOR_NUM = 7;

/**
 * 0: Orange
 * 1: Pink
 * 2: Purple
 * 3: Yellow
 * 4: Red
 * 5: Green
 * 6: Blue
 */

uint32_t colorOrange()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[253]),
        pgm_read_byte(&gamma8[106]),
        pgm_read_byte(&gamma8[2]));
}

uint32_t colorPink()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[231]),
        pgm_read_byte(&gamma8[84]),
        pgm_read_byte(&gamma8[128]));
}

uint32_t colorPurple()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[125]),
        pgm_read_byte(&gamma8[60]),
        pgm_read_byte(&gamma8[152]));
}

uint32_t colorYellow()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]));
}

uint32_t colorGreen()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]));
}

uint32_t colorBlue()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[255]));
}

uint32_t colorGray()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[250]),
        pgm_read_byte(&gamma8[250]),
        pgm_read_byte(&gamma8[250]));
}

const uint32_t LED_STRIP_COLORS[LED_STRIP_COLOR_NUM] = {
    colorOrange(),
    colorPink(),
    colorPurple(),
    colorYellow(),
    colorGray(),
    colorGreen(),
    colorBlue()};

const int LED_STRIP_KEY[LED_STRIP_SUBSTRIP_NUM] = {
    3, 4, 0};

Atm_controller ledStripController;

const unsigned long LED_STRIP_VALID_DELAY_MS = 3500;

/**
 * LED refresh timer.
 */

Atm_timer ledRefreshTimer;

const int LED_REFRESH_TIMER_MS = 150;

/**
 * Relays.
 */

const int RELAY_PIN_THROUGH = A5;
const int RELAY_PIN_STRIP = A4;

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
    unsigned long millisValidStrip;
} ProgramState;

ProgramState progState = {
    .currThroughColorIdx = currThroughColorIdx,
    .currThroughIdx = 0,
    .blinkCounter = 0,
    .currSubstripColorIdx = currSubstripColorIdx,
    .millisValidStrip = 0};

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

    uint32_t color;
    uint8_t limLo;
    uint8_t limHi;

    for (int i = 0; i < LED_STRIP_SUBSTRIP_NUM; i++)
    {
        color = LED_STRIP_COLORS[progState.currSubstripColorIdx[i]];
        limLo = LED_STRIP_SUBSTRIP_LIMITS[i][0];
        limHi = LED_STRIP_SUBSTRIP_LIMITS[i][1];

        for (int j = limLo; j <= limHi; j++)
        {
            ledStrip.setPixelColor(j, color);
        }
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
 * Controller functions.
 */

bool isValidLedThrough()
{
    for (int i = 0; i < LED_THROUGH_NUM; i++)
    {
        if (progState.currThroughColorIdx[i] != LED_THROUGH_KEY[i])
        {
            return false;
        }
    }

    return true;
}

void onValidLedThrough()
{
    Serial.println(F("Valid LED (throughole)"));
    openRelay(RELAY_PIN_THROUGH);
}

void onErrorLedThrough()
{
    Serial.println(F("Error LED (throughole)"));
    lockRelay(RELAY_PIN_THROUGH);
}

bool isValidLedStrip()
{
    for (int i = 0; i < LED_STRIP_SUBSTRIP_NUM; i++)
    {
        if (progState.currSubstripColorIdx[i] != LED_STRIP_KEY[i])
        {
            progState.millisValidStrip = 0;
            return false;
        }
    }

    if (progState.millisValidStrip == 0)
    {
        progState.millisValidStrip = millis();
        return false;
    }
    else
    {
        unsigned long now = millis();

        if (progState.millisValidStrip > now)
        {
            Serial.println(F("Timer overflow"));
            progState.millisValidStrip = 0;
            return false;
        }

        unsigned long diff = now - progState.millisValidStrip;

        return diff > LED_STRIP_VALID_DELAY_MS;
    }
}

void onValidLedStrip()
{
    Serial.println(F("Valid LED (strip)"));
    openRelay(RELAY_PIN_STRIP);
}

void onErrorLedStrip()
{
    Serial.println(F("Error LED (strip)"));
    lockRelay(RELAY_PIN_STRIP);
}

void initControllers()
{
    ledThroughController
        .begin()
        .IF(isValidLedThrough)
        .onChange(true, onValidLedThrough)
        .onChange(false, onErrorLedThrough);

    ledStripController
        .begin()
        .IF(isValidLedStrip)
        .onChange(true, onValidLedStrip)
        .onChange(false, onErrorLedStrip);
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
    initRelay(RELAY_PIN_THROUGH);
    initRelay(RELAY_PIN_STRIP);
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
    initControllers();

    Serial.println(F(">> Starting panel program"));
}

void loop()
{
    automaton.run();
}