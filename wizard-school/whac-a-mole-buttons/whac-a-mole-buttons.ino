#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

/**
 * Buttons.
 */

const int BUTTONS_NUM = 8;

const int BUTTONS_PINS[BUTTONS_NUM] = {
    A0, A1, A2, A3, A4, A5, A6, A7};

Atm_button buttons[BUTTONS_NUM];

const int BUTTONS_BUF_SIZE = 10;
CircularBuffer<byte, BUTTONS_BUF_SIZE> buttonBuf;

const int BUTTONS_BOUNCE_MS = 100;

/**
 * Relay.
 */

const int RELAY_PIN = 3;

/**
 * LED strips.
 */

const int LED_BRIGHTNESS = 230;
const int LED_PIN = 2;
const int LED_NUM = BUTTONS_NUM;
const int LED_ERROR_ITERS = 3;
const int LED_ERROR_SLEEP_MS = 250;
const int LED_SUCCESS_ITERS = 6;
const int LED_SUCCESS_SLEEP_MS = 200;
const int LED_FADE_MS = 10;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_RGB + NEO_KHZ800);

/**
 * Program state.
 */

const int FINAL_PHASE = 4;

const int SUCCESS_STREAK_LONG = 6;
const int SUCCESS_STREAK_MEDIUM = 4;
const int SUCCESS_STREAK_SHORT = 3;

const unsigned long MILLIS_SPAN_LONG = 6000;
const unsigned long MILLIS_SPAN_MEDIUM = 3000;
const unsigned long MILLIS_SPAN_SHORT = 1500;

const int TARGETS_SIZE = BUTTONS_NUM;

int targetKnocks[TARGETS_SIZE];

typedef struct programState
{
    unsigned long startMillis;
    int *targetKnocks;
    int currPhase;
    int hitStreak;
    bool isFinished;
} ProgramState;

ProgramState progState = {
    .startMillis = 0,
    .targetKnocks = targetKnocks,
    .currPhase = 0,
    .hitStreak = 0,
    .isFinished = false};

void cleanState()
{
    buttonBuf.clear();
    emptyTargets();
    progState.startMillis = 0;
    progState.currPhase = 0;
    progState.hitStreak = 0;
    progState.isFinished = false;
}

/**
 * Functions to deal with game state progress.
 */

int getPhaseHitStreak(int phase)
{
    if (phase == 0)
    {
        return SUCCESS_STREAK_LONG;
    }
    else if (phase == 1)
    {
        return SUCCESS_STREAK_MEDIUM;
    }
    else
    {
        return SUCCESS_STREAK_SHORT;
    }
}

unsigned long getPhaseMaxSpanMillis(int phase)
{
    if (phase == 0)
    {
        return MILLIS_SPAN_LONG;
    }
    else if (phase == 1)
    {
        return MILLIS_SPAN_MEDIUM;
    }
    else
    {
        return MILLIS_SPAN_SHORT;
    }
}

int getPhaseNumTargets(int phase)
{
    phase = phase + 1;

    int maxNum = phase + 2;
    int minNum = phase;

    maxNum = maxNum > BUTTONS_NUM ? BUTTONS_NUM : maxNum;
    maxNum = maxNum < 2 ? 2 : maxNum;

    minNum = minNum < 1 ? 1 : minNum;
    minNum = minNum > (BUTTONS_NUM - 1) ? (BUTTONS_NUM - 1) : minNum;

    if (minNum > maxNum)
    {
        minNum = maxNum;
    }

    return random(minNum, maxNum);
}

bool isTarget(int idx)
{
    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        if (progState.targetKnocks[i] == -1)
        {
            return false;
        }
        else if (progState.targetKnocks[i] == idx)
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
        if (progState.targetKnocks[i] == -1)
        {
            Serial.print(F("Adding target: "));
            Serial.println(idx);
            progState.targetKnocks[i] = idx;
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
    int randPivot = random(0, TARGETS_SIZE * 10) % TARGETS_SIZE;
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
            Serial.println(F("Warn: no more random targets to pick"));
            break;
        }

        addTarget(randTarget);
    }
}

bool isButtonBufferError()
{
    for (int i = 0; i < buttonBuf.size(); i++)
    {
        if (!isTarget(buttonBuf[i]))
        {
            return true;
        }
    }

    return false;
}

bool isButtonBufferMatch()
{
    int targetSize = 0;

    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        if (progState.targetKnocks[i] == -1)
        {
            break;
        }

        targetSize++;
    }

    if (targetSize != buttonBuf.size())
    {
        return false;
    }

    bool bufHasTarget;

    for (int i = 0; i < targetSize; i++)
    {
        bufHasTarget = false;

        for (int j = 0; j < buttonBuf.size(); j++)
        {
            if (buttonBuf[j] == progState.targetKnocks[i])
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
    if (progState.startMillis == 0)
    {
        return false;
    }

    unsigned long now = millis();

    if (now < progState.startMillis)
    {
        Serial.println(F("Timer overflow"));
        return true;
    }

    unsigned long maxSpan = getPhaseMaxSpanMillis(progState.currPhase);
    unsigned long limit = progState.startMillis + maxSpan;

    return now > limit;
}

void updateTargets()
{
    clearLeds();
    delay(BUTTONS_BOUNCE_MS);
    int numTargets = getPhaseNumTargets(progState.currPhase);
    randomizeTargets(numTargets);
    showTargetLeds();
    progState.startMillis = millis();
    buttonBuf.clear();
}

void advanceProgress()
{
    progState.hitStreak++;

    int minHitStreak = getPhaseHitStreak(progState.currPhase);

    if (progState.hitStreak >= minHitStreak)
    {
        progState.hitStreak = 0;
        progState.currPhase++;
    }

    updateTargets();
}

bool hasStarted()
{
    return progState.currPhase != 0 || progState.hitStreak != 0;
}

bool hasFinished()
{
    return progState.currPhase >= FINAL_PHASE;
}

void updateState()
{
    if (progState.isFinished)
    {
        fadeLeds();
        return;
    }

    if (hasFinished())
    {
        Serial.println(F("Game completed"));
        progState.isFinished = true;
        showSuccessLedPattern();
        openRelay();
        return;
    }

    if (progState.startMillis == 0)
    {
        Serial.println(F("First target update"));
        updateTargets();
    }
    else if (isButtonBufferError())
    {
        Serial.println(F("Knock pattern error: restart"));

        if (hasStarted())
        {
            showErrorLedsPattern();
        }

        cleanState();
    }
    else if (isExpired())
    {
        Serial.println(F("Time expired: restart"));

        if (hasStarted())
        {
            showErrorLedsPattern();
        }

        cleanState();
    }
    else if (isButtonBufferMatch())
    {
        Serial.println(F("OK: advancing progress"));
        advanceProgress();
    }
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
    int r = randVal == 0 ? 0 : random(100, 250);
    int g = randVal == 1 ? 0 : random(100, 250);
    int b = randVal == 2 ? 0 : random(100, 250);

    return Adafruit_NeoPixel::Color(r, g, b);
}

void showTargetLeds()
{
    ledStrip.clear();

    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        ledStrip.setPixelColor(progState.targetKnocks[i], randomColor());
    }

    ledStrip.show();
}

void showErrorLedsPattern()
{
    for (int i = 0; i < LED_ERROR_ITERS; i++)
    {
        for (int j = 0; j < LED_NUM; j++)
        {
            ledStrip.setPixelColor(j, 255, 0, 0);
        }

        ledStrip.show();

        delay(LED_ERROR_SLEEP_MS);

        ledStrip.clear();
        ledStrip.show();

        delay(LED_ERROR_SLEEP_MS);
    }
}

void showSuccessLedPattern()
{
    clearLeds();

    for (int i = 0; i < LED_SUCCESS_ITERS; i++)
    {
        for (int j = 0; j < LED_NUM; j++)
        {
            ledStrip.setPixelColor(j, randomColor());
            ledStrip.show();
            delay(LED_SUCCESS_SLEEP_MS);
            clearLeds();
        }
    }

    clearLeds();
}

void setPixelColorChannel(int idx, int channel, int val)
{
    if (channel == 0)
    {
        ledStrip.setPixelColor(idx, val, 0, 0);
    }
    else if (channel == 1)
    {
        ledStrip.setPixelColor(idx, 0, val, 0);
    }
    else
    {
        ledStrip.setPixelColor(idx, 0, 0, val);
    }
}

void fadeLeds()
{
    clearLeds();

    int channel = random(0, 3);

    for (int i = 0; i < 255; i++)
    {
        for (int j = 0; j < LED_NUM; j++)
        {
            setPixelColorChannel(j, channel, i);
        }

        ledStrip.show();
        delay(LED_FADE_MS);
    }

    for (int i = 255; i >= 0; i--)
    {
        for (int j = 0; j < LED_NUM; j++)
        {
            setPixelColorChannel(j, channel, i);
        }

        ledStrip.show();
        delay(LED_FADE_MS);
    }

    clearLeds();
}

/**
 * Relay functions.
 */

void lockRelay()
{
    digitalWrite(RELAY_PIN, LOW);
}

void openRelay()
{
    digitalWrite(RELAY_PIN, HIGH);
}

void initRelay()
{
    pinMode(RELAY_PIN, OUTPUT);
    lockRelay();
}

/**
 * Knock sensor functions.
 */

void onPress(int idx, int v, int up)
{
    Serial.print(F("Press:"));
    Serial.println(idx);

    bool isDup = false;

    for (int i = 0; i < buttonBuf.size(); i++)
    {
        if (buttonBuf[i] == idx)
        {
            isDup = true;
            break;
        }
    }

    if (!isDup)
    {
        Serial.print(F("Pushing:"));
        Serial.println(idx);

        buttonBuf.push(idx);
    }
}

void initButtons()
{
    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        buttons[i]
            .begin(BUTTONS_PINS[i])
            .onPress(onPress, i);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initButtons();
    initLeds();
    initRelay();

    Serial.println(F(">> Starting whac-a-mole program"));
}

void loop()
{
    automaton.run();
    updateState();
}
