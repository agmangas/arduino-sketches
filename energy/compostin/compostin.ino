#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

/**
 * Buttons.
 */

const int BUTTONS_NUM = 8;

const int BUTTONS_PINS[BUTTONS_NUM] = {
    4, 5, 6, 7, 8, 9, 10, 11};

Atm_button buttons[BUTTONS_NUM];

const int BUTTONS_BUF_SIZE = 10;
CircularBuffer<int, BUTTONS_BUF_SIZE> buttonBuf;

/**
 * LED strips.
 */

const int LED_BRIGHTNESS = 230;
const int LED_PIN = 2;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(
    BUTTONS_NUM, LED_PIN, NEO_RGB + NEO_KHZ800);

const int LED_COLOR_PALETTE_SIZE = 6;

const uint32_t LED_COLOR_PALETTE[LED_COLOR_PALETTE_SIZE] = {
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 0, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 255, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 0, 255)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 255, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 0, 255)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 255, 255))};

/**
 * Relay.
 */

const int RELAY_PIN = 3;

/**
 * Program state.
 */

Atm_timer timerState;

const int STATE_TIMER_MS = 50;

const int FINAL_PHASE = 4;

int targetButtonIdxs[BUTTONS_NUM];
uint32_t targetColors[BUTTONS_NUM];

typedef struct programState
{
    unsigned long startMillis;
    int *targetButtonIdxs;
    uint32_t *targetColors;
    int currPhase;
    int hitStreak;
    bool isFinished;
} ProgramState;

ProgramState progState;

void initState()
{
    buttonBuf.clear();

    emptyTargets();

    progState.startMillis = 0;
    progState.targetButtonIdxs = targetButtonIdxs;
    progState.targetColors = targetColors;
    progState.currPhase = 0;
    progState.hitStreak = 0;
    progState.isFinished = false;
}

/**
 * Functions to interface with the targets array.
 */

void emptyTargets()
{
    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        targetButtonIdxs[i] = -1;
        targetColors[i] = 0;
    }
}

bool isTarget(int idx)
{
    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        if (progState.targetButtonIdxs[i] == -1)
        {
            return false;
        }
        else if (progState.targetButtonIdxs[i] == idx)
        {
            return true;
        }
    }

    return false;
}

bool pushTarget(int idx)
{
    if (idx < 0 || idx >= BUTTONS_NUM)
    {
        Serial.println(F("Target should be in [0, numLeds)"));
        return false;
    }

    if (isTarget(idx))
    {
        return false;
    }

    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        if (progState.targetButtonIdxs[i] == -1)
        {
            Serial.print(F("Adding target: "));
            Serial.println(idx);
            progState.targetButtonIdxs[i] = idx;
            return true;
        }
    }

    return false;
}

int pickRandomTarget()
{
    int randPivot = random(0, BUTTONS_NUM * 10) % BUTTONS_NUM;
    int counter = 0;

    while (isTarget(randPivot) && counter <= BUTTONS_NUM)
    {
        randPivot = (randPivot + 1) % BUTTONS_NUM;
        counter++;
    }

    return isTarget(randPivot) ? -1 : randPivot;
}

void randomizeTargets(int num)
{
    num = (num > (BUTTONS_NUM)) ? BUTTONS_NUM : num;

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

        pushTarget(randTarget);
    }
}

void updateColorsFromTargets()
{
    uint32_t color;

    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        color = isTarget(i)
                    ? getPhaseRandomColorValid(progState.currPhase)
                    : getPhaseRandomColorError(progState.currPhase);

        progState.targetColors[i] = color;
    }
}

/**
 * Functions to interface with the button presses buffer.
 */

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

bool inButtonBuffer(int val)
{
    for (int i = 0; i < buttonBuf.size(); i++)
    {
        if (buttonBuf[i] == val)
        {
            return true;
        }
    }

    return false;
}

bool isButtonBufferMatch()
{
    int targetSize = 0;

    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        if (progState.targetButtonIdxs[i] == -1)
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
            if (buttonBuf[j] == progState.targetButtonIdxs[i])
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

/**
 * Functions to get the dynamic config that depends on the current phase.
 */

int getPhaseHitStreak(int phase)
{
    const int streakLong = 6;
    const int streakMedium = 4;
    const int streakShort = 3;

    if (phase == 0)
    {
        return streakLong;
    }
    else if (phase == 1)
    {
        return streakMedium;
    }
    else
    {
        return streakShort;
    }
}

unsigned long getPhaseMaxSpanMillis(int phase)
{
    const unsigned long millisLong = 6000;
    const unsigned long millisMedium = 3000;
    const unsigned long millisShort = 1500;

    if (phase == 0)
    {
        return millisLong;
    }
    else if (phase == 1)
    {
        return millisMedium;
    }
    else
    {
        return millisShort;
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

int getPhaseColorDivider(int phase)
{
    const int idxOne = 1;
    const int idxFew = 2;
    const int idxLots = 3;

    if (phase == 0)
    {
        return idxOne;
    }
    else if (phase == 1)
    {
        return idxFew;
    }
    else
    {
        return idxLots;
    }
}

uint32_t getPhaseRandomColorValid(int phase)
{
    int idxDivider = getPhaseColorDivider(phase);
    int idxColor = random(0, idxDivider);

    return LED_COLOR_PALETTE[idxColor];
}

uint32_t getPhaseRandomColorError(int phase)
{
    int idxDivider = getPhaseColorDivider(phase);
    int idxColor = random(idxDivider, LED_COLOR_PALETTE_SIZE);

    return LED_COLOR_PALETTE[idxColor];
}

/**
 * Functions to deal with game state progress.
 */

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
    int numTargets = getPhaseNumTargets(progState.currPhase);
    randomizeTargets(numTargets);
    updateColorsFromTargets();
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
}

bool hasStarted()
{
    return progState.currPhase != 0 || progState.hitStreak != 0;
}

bool hasFinished()
{
    return progState.currPhase >= FINAL_PHASE;
}

void errorAndRestart()
{
    if (hasStarted())
    {
        showErrorLedsPattern();
    }

    initState();
}

void updateState()
{
    if (progState.isFinished)
    {
        return;
    }

    if (hasFinished())
    {
        Serial.println(F("Game completed"));
        progState.isFinished = true;
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
        errorAndRestart();
    }
    else if (isExpired())
    {
        Serial.println(F("Time expired: restart"));
        errorAndRestart();
    }
    else if (isButtonBufferMatch())
    {
        Serial.println(F("OK: advancing progress"));
        advanceProgress();
        updateTargets();
    }
    else
    {
        showTargetLeds();
    }
}

/**
 * LED functions.
 */

void initLeds()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_BRIGHTNESS);
    ledStrip.clear();
    ledStrip.show();
}

void showErrorLedsPattern()
{
    const int numIters = 3;
    const int delayMs = 250;
    const uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);

    for (int i = 0; i < numIters; i++)
    {
        ledStrip.fill(red);
        ledStrip.show();

        delay(delayMs);

        ledStrip.clear();
        ledStrip.show();

        delay(delayMs);
    }
}

void showTargetLeds()
{
    uint32_t color;

    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        color = inButtonBuffer(i) ? 0 : progState.targetColors[i];
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

void onStateTimer(int idx, int v, int up)
{
    updateState();
}

void initStateTimer()
{
    timerState
        .begin(STATE_TIMER_MS)
        .repeat(-1)
        .onTimer(onStateTimer)
        .start();
}

void setup()
{
    Serial.begin(9600);

    initState();
    initButtons();
    initLeds();
    initRelay();
    initStateTimer();

    Serial.println(F(">> Starting whac-a-mole program"));
}

void loop()
{
    automaton.run();
}
