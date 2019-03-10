#include <Adafruit_NeoPixel.h>
#include <Automaton.h>

/**
 * Potentiometers.
 */

const int POTS_NUM = 4;

const int POTS_PINS[POTS_NUM] = {
    A7, A6, A5, A4};

Atm_analog pots[POTS_NUM];

const byte POTS_RANGE_LO = 1;
const byte POTS_RANGE_HI = 20;

/**
 * LED strip.
 */

const uint16_t LEDS_NUM = 300;
const uint8_t LEDS_PIN = 6;
const int LEDS_BRIGHTNESS = 150;

const int LEDS_POTS_SEGMENTS[POTS_NUM][2] = {
    {1, 20},
    {40, 21},
    {41, 60},
    {80, 61}};

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(LEDS_NUM, LEDS_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

int currPotValues[POTS_NUM];

typedef struct programState
{
    bool isSecondPhaseUnlocked;
    int *currPotValues;
} ProgramState;

ProgramState progState = {
    .isSecondPhaseUnlocked = false,
    .currPotValues = currPotValues};

/**
 * Potentiometer functions.
 */

void onPotChange(int idx, int v, int up)
{
    Serial.print(F("Pot["));
    Serial.print(idx);
    Serial.print(F("]:"));
    Serial.println(v);

    progState.currPotValues[idx] = v;

    refreshLedPotSegments();
}

void initPots()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        progState.currPotValues[i] = POTS_RANGE_LO;
    }

    for (int i = 0; i < POTS_NUM; i++)
    {
        pots[i]
            .begin(POTS_PINS[i])
            .range(POTS_RANGE_LO, POTS_RANGE_HI)
            .onChange(onPotChange, i);
    }
}

/**
 * LED strip functions.
 */

uint32_t randomColor()
{
    return Adafruit_NeoPixel::Color(random(100, 250), 0, 0);
}

void refreshLedPotSegments()
{
    const int SEGMENT_SIZE = POTS_RANGE_HI - POTS_RANGE_LO + 1;

    bool isAsc;
    int iniLed;
    int endLed;

    for (int i = 0; i < POTS_NUM; i++)
    {
        isAsc = LEDS_POTS_SEGMENTS[i][1] >
                LEDS_POTS_SEGMENTS[i][0];

        iniLed = isAsc ? LEDS_POTS_SEGMENTS[i][0] : LEDS_POTS_SEGMENTS[i][1];
        endLed = iniLed + progState.currPotValues[i];

        for (int i = iniLed; i < (iniLed + SEGMENT_SIZE); i++)
        {
            pixelStrip.setPixelColor(i, 0);
        }

        for (int i = iniLed; i < endLed; i++)
        {
            pixelStrip.setPixelColor(i, randomColor());
        }
    }

    pixelStrip.show();
}

void initLeds()
{
    pixelStrip.begin();
    pixelStrip.setBrightness(LEDS_BRIGHTNESS);
    pixelStrip.show();

    clearLeds();
    refreshLedPotSegments();
}

void clearLeds()
{
    pixelStrip.clear();
    pixelStrip.show();
}

/**
 * Configuration validation functions.
 */

void validateLedSegments()
{
    const int DIFF_EXPECTED = POTS_RANGE_HI - POTS_RANGE_LO;

    bool isAsc;
    int diff;

    for (int i = 0; i < POTS_NUM; i++)
    {
        isAsc = LEDS_POTS_SEGMENTS[i][1] >
                LEDS_POTS_SEGMENTS[i][0];

        if (isAsc)
        {
            diff = LEDS_POTS_SEGMENTS[i][1] -
                   LEDS_POTS_SEGMENTS[i][0];
        }
        else
        {
            diff = LEDS_POTS_SEGMENTS[i][0] -
                   LEDS_POTS_SEGMENTS[i][1];
        }

        if (diff != DIFF_EXPECTED)
        {
            Serial.println(F("#####"));
            Serial.println(F("## WARNING: Invalid LED segments"));
            Serial.println(F("#####"));

            signalProgramError();
        }
    }
}

void signalProgramError()
{
    const int ERR_LED_DELAY_MS = 500;

    pinMode(LED_BUILTIN, OUTPUT);

    while (true)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(ERR_LED_DELAY_MS);
        digitalWrite(LED_BUILTIN, LOW);
        delay(ERR_LED_DELAY_MS);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initPots();
    initLeds();
    validateLedSegments();

    Serial.println(F(">> Starting incubator program"));
}

void loop()
{
    automaton.run();
}