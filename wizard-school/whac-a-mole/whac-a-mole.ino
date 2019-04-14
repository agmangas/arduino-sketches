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

const unsigned long MILLIS_SPAN_LONG = 5000;
const unsigned long MILLIS_SPAN_MEDIUM = 3000;
const unsigned long MILLIS_SPAN_SHORT = 1500;

const int TARGETS_SIZE = KNOCK_NUM;

int targetKnocks[TARGETS_SIZE];

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

unsigned long getCurrentMaxSpanMillis()
{
    if (progState.currPhase == 0)
    {
        return MILLIS_SPAN_LONG;
    }
    else if (progState.currPhase == 1)
    {
        return MILLIS_SPAN_MEDIUM;
    }
    else
    {
        return MILLIS_SPAN_SHORT;
    }
}

bool isTarget(int idx)
{
    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        if (targetKnocks[i] == -1)
        {
            return false;
        }
        else if (targetKnocks[i] == idx)
        {
            return true;
        }
    }

    return false;
}

bool addTarget(int idx)
{
    if (idx < 0 || idx >= LED_NUM)
    {
        Serial.println(F("Target should be in [0, numLeds)"));
        return false;
    }

    if (isTarget(idx))
    {
        return false;
    }

    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        if (targetKnocks[i] == -1)
        {
            targetKnocks[i] = idx;
            return true;
        }
    }

    return false;
}

void emptyTargets()
{
    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        progState.targetKnocks[i] = -1;
    }
}

int pickRandomTarget()
{
    int randPivot = random(0, 100) % TARGETS_SIZE;
    int counter = 0;

    while (isTarget(randPivot) && counter <= TARGETS_SIZE)
    {
        randPivot = (randPivot + 1) % TARGETS_SIZE;
        counter++;
    }

    return isTarget(randPivot) ? -1 : randPivot;
}

void randomizeTargets(int num)
{
    num = (num > (TARGETS_SIZE)) ? TARGETS_SIZE : num;

    emptyTargets();

    int randTarget;

    for (int i = 0; i < num; i++)
    {
        randTarget = pickRandomTarget();

        if (randTarget == -1)
        {
            break;
        }

        addTarget(randTarget);
    }
}

bool isKnockBufferError()
{
    for (int i = 0; i < knockBuf.size(); i++)
    {
        if (!isTarget(knockBuf[i]))
        {
            return true;
        }
    }

    return false;
}

bool isKnockBufferMatch()
{
    int targetSize = 0;

    for (int i = 0; i < TARGETS_SIZE; i++)
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

    for (int i = 0; i < targetSize; i++)
    {
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

uint32_t randomColor()
{
    int randVal = random(0, 3);
    int r = randVal == 0 ? 0 : random(25, 250);
    int g = randVal == 1 ? 0 : random(25, 250);
    int b = randVal == 2 ? 0 : random(25, 250);

    return Adafruit_NeoPixel::Color(r, g, b);
}

void showTargetLeds()
{
    ledStrip.clear();

    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        ledStrip.setPixelColor(targetKnocks[i], randomColor());
    }

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