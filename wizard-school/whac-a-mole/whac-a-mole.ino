#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

/**
 * Piezo knock sensors.
 */

const int KNOCK_NUM = 8;
const int KNOCK_SAMPLERATE = 50;
const int KNOCK_RANGE_MIN = 0;
const int KNOCK_RANGE_MAX = 100;
const int KNOCK_THRESHOLD = 8;

const int KNOCK_PINS[KNOCK_NUM] = {
    A0, A1, A2, A3, A4, A5, A6, A7};

Atm_analog knockAnalogs[KNOCK_NUM];
Atm_controller knockControllers[KNOCK_NUM];

const int KNOCK_BUF_SIZE = 10;
CircularBuffer<byte, KNOCK_BUF_SIZE> knockBuf;

/**
   LED strips.
*/

const int LED_BRIGHTNESS = 200;
const int LED_PIN = 2;
const int LED_NUM = KNOCK_NUM;

const uint32_t LED_COLOR = Adafruit_NeoPixel::Color(100, 255, 0);

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

int targetKnocks[KNOCK_NUM];

typedef struct programState
{
    unsigned long startMillis;
    unsigned long maxSpanMillis;
    int *targetKnocks;
    int currPhase;
} ProgramState;

ProgramState progState = {
    .startMillis = 0,
    .maxSpanMillis = 0,
    .targetKnocks = targetKnocks,
    .currPhase = 0};

/**
 * Knock state functions.
 */

void randomizeTargets()
{
    int numParallel = (currPhase >= (KNOCK_NUM)) ? KNOCK_NUM : currPhase;
}

bool isKnockBufferValid()
{
    int targetSize = 0;

    for (int i = 0; i < KNOCK_NUM; i++)
    {
        if (targetKnocks[i] == -1)
        {
            break;
        }

        targetSize++;
    }

    if (targetSize != knockBuf.size())
    {
        return false;
    }

    bool bufHasTarget;

    for (int i = 0; i < KNOCK_NUM; i++)
    {
        if (targetKnocks[i] == -1)
        {
            break;
        }

        bufHasTarget = false;

        for (int j = 0; j < knockBuf.size(); j++)
        {
            if (knockBuf[j] == targetKnocks[i])
            {
                bufHasTarget = true;
                break;
            }
        }

        if (!bufHasTarget)
        {
            return false;
        }
    }

    return true;
}

bool isExpired()
{
    if (progState.startMillis == 0 || progState.maxSpanMillis == 0)
    {
        return false;
    }

    unsigned long now = millis();

    if (now < progState.startMillis)
    {
        Serial.println(F("Timer overflow"));
        return true;
    }

    unsigned long limit = progState.startMillis + progState.maxSpanMillis;

    return now > limit;
}

/**
 * LED functions.
 */

void initLeds()
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

/**
 * Knock sensor functions.
 */

void onKnock(int idx, int v, int up)
{
    Serial.print(F("Knock:"));
    Serial.println(idx);

    bool isDup = false;

    for (int i = 0; i < knockBuf.size(); i++)
    {
        if (knockBuf[i] == idx)
        {
            isDup = true;
            break;
        }
    }

    if (!isDup)
    {
        knockBuf.push(idx);
    }

    ledStrip.setPixelColor(idx, LED_COLOR);
    ledStrip.show();

    delay(5);

    ledStrip.setPixelColor(idx, 0);
    ledStrip.show();
}

void initKnockSensors()
{
    for (int i = 0; i < KNOCK_NUM; i++)
    {
        knockAnalogs[i]
            .begin(KNOCK_PINS[i], KNOCK_SAMPLERATE)
            .range(KNOCK_RANGE_MIN, KNOCK_RANGE_MAX);

        knockControllers[i]
            .begin()
            .IF(knockAnalogs[i], '>', KNOCK_THRESHOLD)
            .onChange(true, onKnock, i);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initKnockSensors();
    initLeds();

    Serial.println(F(">> Starting whac-a-mole program"));
}

void loop()
{
    automaton.run();
}