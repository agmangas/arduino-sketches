#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

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

uint32_t colorRed()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]),
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
        pgm_read_byte(&gamma8[200]),
        pgm_read_byte(&gamma8[200]),
        pgm_read_byte(&gamma8[200]));
}

uint32_t randomColor()
{
    int randVal = random(0, 3);
    int r = randVal == 0 ? 0 : random(100, 250);
    int g = randVal == 1 ? 0 : random(100, 250);
    int b = randVal == 2 ? 0 : random(100, 250);

    return Adafruit_NeoPixel::Color(r, g, b);
}

/**
 * Piezo knock sensors.
 */

const int KNOCK_NUM = 7;
const int KNOCK_SAMPLERATE = 50;
const int KNOCK_RANGE_MIN = 0;
const int KNOCK_RANGE_MAX = 100;
const int KNOCK_THRESHOLD = 10;

const int KNOCK_PINS[KNOCK_NUM] = {
    A0, A1, A2, A3, A4, A5, A7};

Atm_analog knockAnalogs[KNOCK_NUM];
Atm_controller knockControllers[KNOCK_NUM];

const int KNOCK_BUF_SIZE = 10;
CircularBuffer<byte, KNOCK_BUF_SIZE> knockBuf;

const int KNOCK_BOUNCE_MS = 500;

/**
 * This knock sensor does not seem to work properly.
 * We'll ignore it for the time being.
 */

const bool KNOCK_IGNORE = true;
const int KNOCK_IGNORED_IDX = 3;

/**
 * Colors for the unlock phase.
 */

const int UNLOCK_COLOR_NUM = 7;
const unsigned long UNLOCK_DEBOUNCE_MS = 200;

const uint32_t UNLOCK_COLORS[UNLOCK_COLOR_NUM] = {
    colorBlue(),
    colorRed(),
    colorGreen(),
    colorYellow(),
    colorPink(),
    colorPurple(),
    colorGray()};

// Blue, Red, Green, Yellow, Pink, Purple, Gray

const int UNLOCK_COLORS_KEY[KNOCK_NUM] = {
    2, 2, 2, 2, 2, 2, 2};

/**
 * Relay.
 */

const int RELAY_PIN = 3;

/**
 * LED strips.
 */

const int LED_BRIGHTNESS = 230;
const int LED_PIN = 2;
const int LED_NUM = KNOCK_NUM;
const int LED_ERROR_ITERS = 3;
const int LED_ERROR_SLEEP_MS = 250;
const int LED_SUCCESS_ITERS = 6;
const int LED_SUCCESS_SLEEP_MS = 200;
const int LED_FADE_MS = 5;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_RGB + NEO_KHZ800);

/**
 * Program state.
 */

const int FINAL_PHASE = 4;

const int SUCCESS_STREAK_LONG = 6;
const int SUCCESS_STREAK_MEDIUM = 4;
const int SUCCESS_STREAK_SHORT = 3;

const unsigned long MILLIS_SPAN_LONG = 6000;
const unsigned long MILLIS_SPAN_MEDIUM = 6000;
const unsigned long MILLIS_SPAN_SHORT = 6000;

const int TARGETS_SIZE = KNOCK_NUM;

int targetKnocks[TARGETS_SIZE];
int currColorIdx[KNOCK_NUM];

typedef struct programState
{
    bool isKnockUnlocked;
    unsigned long lastLockMillis;
    unsigned long startMillis;
    int *targetKnocks;
    int currPhase;
    int hitStreak;
    bool isKnockComplete;
    int *currColorIdx;
} ProgramState;

ProgramState progState = {
    .isKnockUnlocked = false,
    .lastLockMillis = 0,
    .startMillis = 0,
    .targetKnocks = targetKnocks,
    .currPhase = 0,
    .hitStreak = 0,
    .isKnockComplete = false,
    .currColorIdx = currColorIdx};

void initState()
{
    for (int i = 0; i < KNOCK_NUM; i++)
    {
        progState.currColorIdx[i] = 0;
    }

    cleanKnockGameState();
}

void cleanKnockGameState()
{
    knockBuf.clear();
    emptyTargets();
    progState.startMillis = 0;
    progState.currPhase = 0;
    progState.hitStreak = 0;
    progState.isKnockComplete = false;
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

    maxNum = maxNum > KNOCK_NUM ? KNOCK_NUM : maxNum;
    maxNum = maxNum < 2 ? 2 : maxNum;

    minNum = minNum < 1 ? 1 : minNum;
    minNum = minNum > (KNOCK_NUM - 1) ? (KNOCK_NUM - 1) : minNum;

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
    int pivot = random(0, TARGETS_SIZE * 10) % TARGETS_SIZE;
    int counter = 0;

    while ((isTarget(pivot) || isIgnoredKnock(pivot)) &&
           counter <= TARGETS_SIZE)
    {
        pivot = (pivot + 1) % TARGETS_SIZE;
        counter++;
    }

    return (isTarget(pivot) || isIgnoredKnock(pivot)) ? -1 : pivot;
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
        if (progState.targetKnocks[i] == -1)
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
            if (knockBuf[j] == progState.targetKnocks[i])
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
    delay(KNOCK_BOUNCE_MS);
    int numTargets = getPhaseNumTargets(progState.currPhase);
    randomizeTargets(numTargets);
    showTargetLeds();
    progState.startMillis = millis();
    knockBuf.clear();
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

bool hasReachedFinalKnockPhase()
{
    return progState.currPhase >= FINAL_PHASE;
}

bool isValidUnlockCombination()
{
    for (int i = 0; i < KNOCK_NUM; i++)
    {
        if (!isIgnoredKnock(i) &&
            progState.currColorIdx[i] != UNLOCK_COLORS_KEY[i])
        {
            return false;
        }
    }

    return true;
}

void updateState()
{
    if (progState.isKnockComplete)
    {
        fadeLeds();
        return;
    }

    if (!progState.isKnockUnlocked)
    {
        refreshUnlockPhaseLeds();

        if (isValidUnlockCombination())
        {
            Serial.println(F("Valid unlock combination"));
            fadeLeds();
            progState.isKnockUnlocked = true;
        }

        return;
    }

    if (hasReachedFinalKnockPhase())
    {
        Serial.println(F("Game complete"));
        progState.isKnockComplete = true;
        showSuccessLedPattern();
        openRelay();
        return;
    }

    if (progState.startMillis == 0)
    {
        Serial.println(F("First target update"));
        updateTargets();
    }
    else if (isKnockBufferError())
    {
        Serial.println(F("Knock pattern error: restart"));

        if (hasStarted())
        {
            showErrorLedsBlink();
        }

        cleanKnockGameState();
    }
    else if (isExpired())
    {
        Serial.println(F("Time expired: restart"));

        if (hasStarted())
        {
            uint32_t blinkColor = Adafruit_NeoPixel::Color(255, 0, 0);
            showErrorLedsBlink(blinkColor);
        }

        cleanKnockGameState();
    }
    else if (isKnockBufferMatch())
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

void showTargetLeds()
{
    ledStrip.clear();

    for (int i = 0; i < TARGETS_SIZE; i++)
    {
        ledStrip.setPixelColor(progState.targetKnocks[i], colorPurple());
    }

    ledStrip.show();
}

void showErrorLedsBlink()
{
    showErrorLedsBlink(Adafruit_NeoPixel::Color(255, 0, 0));
}

void showErrorLedsBlink(uint32_t color)
{
    for (int i = 0; i < LED_ERROR_ITERS; i++)
    {
        for (int j = 0; j < LED_NUM; j++)
        {
            ledStrip.setPixelColor(j, color);
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

void refreshUnlockPhaseLeds()
{
    ledStrip.clear();

    for (int i = 0; i < KNOCK_NUM; i++)
    {
        uint32_t color = UNLOCK_COLORS[progState.currColorIdx[i]];

        if (isIgnoredKnock(i))
        {
            color = 0;
        }

        ledStrip.setPixelColor(i, color);
    }

    ledStrip.show();
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

bool isUnlockKnockEnabled()
{
    unsigned long now = millis();
    return (now - progState.lastLockMillis) > UNLOCK_DEBOUNCE_MS;
}

bool isIgnoredKnock(int knockIdx)
{
    return KNOCK_IGNORE && knockIdx == KNOCK_IGNORED_IDX;
}

void onUnlockPhaseKnock(int idx)
{
    if (!isUnlockKnockEnabled())
    {
        return;
    }

    Serial.print(F("## Knock::Unlock:"));
    Serial.println(idx);

    progState.lastLockMillis = millis();

    progState.currColorIdx[idx]++;
    progState.currColorIdx[idx] = progState.currColorIdx[idx] % UNLOCK_COLOR_NUM;
}

void onKnock(int idx, int v, int up)
{
    if (isIgnoredKnock(idx))
    {
        Serial.print(F("Ignored knock"));
        return;
    }

    if (!progState.isKnockUnlocked)
    {
        onUnlockPhaseKnock(idx);
        return;
    }

    int analogVal = knockAnalogs[idx].state();

    Serial.print(F("## Knock:"));
    Serial.print(idx);
    Serial.print(":");
    Serial.println(analogVal);

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
        Serial.print(F("Pushing:"));
        Serial.println(idx);

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

    initState();
    initKnockSensors();
    initLeds();
    initRelay();

    Serial.println(F(">> Starting whac-a-mole program"));
}

void loop()
{
    automaton.run();
    updateState();
}
