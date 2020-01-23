#include <Adafruit_NeoPixel.h>
#include <Automaton.h>
#include <CircularBuffer.h>
#include <limits.h>

/**
 * Buttons.
 */

const int BUTTONS_NUM = 8;

const int BUTTONS_PINS[BUTTONS_NUM] = {
    2, 3, 4, 5, 6, 7, 8, 9
};

Atm_button buttons[BUTTONS_NUM];

/**
 * LED strips.
 */

const int LED_BRIGHTNESS = 200;
const int LED_PIN = 10;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(
    BUTTONS_NUM,
    LED_PIN,
    NEO_RGB + NEO_KHZ800);

const int LED_COLOR_PALETTE_SIZE = 2;
const int LED_COLOR_PALETTE_NONE_IDX = 0;

const uint32_t LED_COLOR_PALETTE[LED_COLOR_PALETTE_SIZE] = {
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 0, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(125, 60, 152))
};

CircularBuffer<uint8_t, LED_COLOR_PALETTE_SIZE> bufErrorPalette;

/**
 * Colors for the unlock phase.
 */

const int UNLOCK_COLOR_NUM = 7;

const uint32_t UNLOCK_COLORS[UNLOCK_COLOR_NUM] = {
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 0, 255)), // 0: Blue
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 0, 0)), // 1: Red
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 255, 0)), // 2: Green
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 255, 0)), // 3: Yellow
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(231, 84, 128)), // 4: Pink
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(125, 60, 152)), // 5: Purple
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(200, 200, 200)) // 6: Grey
};

const int UNLOCK_COLORS_KEY[BUTTONS_NUM] = {
    1, 0, 1, 2, 4, 2, 1, 0
};

/**
 * Relay.
 */

const int RELAY_PIN = 11;

/**
 * Constant that determines the final phase 
 * (a phase is a series of stages where the same amount of buttons light up).
 */

const int FINAL_PHASE = 3;

/**
 * Program state.
 */

Atm_timer timerState;

const int STATE_TIMER_MS = 50;
const int BUTTONS_BUF_SIZE = 10;

CircularBuffer<int, BUTTONS_BUF_SIZE> bufButtonPresses;
CircularBuffer<int, BUTTONS_NUM> bufButtonTargets;
CircularBuffer<uint32_t, BUTTONS_NUM> bufButtonColors;

int unlockColorIdx[BUTTONS_NUM];

typedef struct programState {
    unsigned long startMillis;
    int currPhase;
    int hitStreak;
    bool isFinished;
    bool isUnlocked;
    int* unlockColorIdx;
} ProgramState;

ProgramState progState;

/**
 * Utility to check for existence in a circular buffer.
 */

template <typename T, size_t S>
bool inBuffer(const CircularBuffer<T, S>& buf, T val)
{
    for (int i = 0; i < buf.size(); i++) {
        if (buf[i] == val) {
            return true;
        }
    }

    return false;
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
 * Functions to get the dynamic config that depends on the current phase.
 */

int getPhaseHitStreak(int phase)
{
    const int streakLong = 6;
    const int streakMedium = 4;
    const int streakShort = 3;

    if (phase == 0) {
        return streakLong;
    } else if (phase == 1) {
        return streakMedium;
    } else {
        return streakShort;
    }
}

unsigned long getPhaseMaxSpanMillis(int phase)
{
    const unsigned long millisLong = 6000;
    const unsigned long millisMedium = 5000;
    const unsigned long millisShort = 4000;

    if (phase == 0) {
        return millisLong;
    } else if (phase == 1) {
        return millisMedium;
    } else {
        return millisShort;
    }
}

int getPhaseNumTargets(int phase)
{
    const int minNum = 2;
    const int maxNum = 5;

    if (maxNum > BUTTONS_NUM) {
        Serial.println(F("WARN :: Unexpected number of buttons"));
        return random(1, BUTTONS_NUM);
    } else {
        return random(minNum, maxNum);
    }
}

void refreshErrorPaletteBuffer(int phase)
{
    bufErrorPalette.clear();
    bufErrorPalette.push(LED_COLOR_PALETTE_NONE_IDX);
}

int getRandomColorIndex(int phase, bool isError)
{
    refreshErrorPaletteBuffer(phase);

    uint8_t idx;
    bool found;

    do {
        idx = random(0, LED_COLOR_PALETTE_SIZE);
        found = inBuffer(bufErrorPalette, idx);
    } while (found != isError);

    return idx;
}

uint32_t getPhaseRandomColorValid(int phase)
{
    int idx = getRandomColorIndex(phase, false);
    return LED_COLOR_PALETTE[idx];
}

uint32_t getPhaseRandomColorError(int phase)
{
    int idx = getRandomColorIndex(phase, true);
    return LED_COLOR_PALETTE[idx];
}

/**
 * Functions to interface with the targets array.
 */

void clearTargets()
{
    bufButtonTargets.clear();
    bufButtonColors.clear();
}

bool inTargetsBuffer(int idx)
{
    return inBuffer(bufButtonTargets, idx);
}

bool pushTarget(int idx)
{
    if (idx < 0 || idx >= BUTTONS_NUM) {
        Serial.println(F("Target should be in [0, numLeds)"));
        return false;
    }

    if (inTargetsBuffer(idx)) {
        return false;
    }

    if (!bufButtonTargets.isFull()) {
        Serial.print(F("Adding target: "));
        Serial.println(idx);
        bufButtonTargets.push(idx);
        return true;
    }

    return false;
}

int pickRandomTarget()
{
    int randPivot = random(0, BUTTONS_NUM * 10) % BUTTONS_NUM;
    int counter = 0;

    while (inTargetsBuffer(randPivot) && counter <= BUTTONS_NUM) {
        randPivot = (randPivot + 1) % BUTTONS_NUM;
        counter++;
    }

    return inTargetsBuffer(randPivot) ? -1 : randPivot;
}

void randomizeTargets(int num)
{
    num = (num > (BUTTONS_NUM)) ? BUTTONS_NUM : num;

    clearTargets();

    int randTarget;

    for (int i = 0; i < num; i++) {
        randTarget = pickRandomTarget();

        if (randTarget == -1) {
            Serial.println(F("Warn: no more random targets to pick"));
            break;
        }

        pushTarget(randTarget);
    }
}

void updateButtonColorsBuffer()
{
    uint32_t color;

    bufButtonColors.clear();

    for (int i = 0; i < BUTTONS_NUM; i++) {
        color = inTargetsBuffer(i)
            ? getPhaseRandomColorValid(progState.currPhase)
            : getPhaseRandomColorError(progState.currPhase);

        bufButtonColors.push(color);
    }
}

/**
 * Function to reset state.
 */

void initState()
{
    bufButtonPresses.clear();

    clearTargets();

    progState.startMillis = 0;
    progState.currPhase = 0;
    progState.hitStreak = 0;
    progState.isFinished = false;
    progState.isUnlocked = false;
    progState.unlockColorIdx = unlockColorIdx;

    for (int i = 0; i < BUTTONS_NUM; i++) {
        progState.unlockColorIdx[i] = 0;
    }
}

void resetStateToPhaseStart()
{
    bufButtonPresses.clear();

    clearTargets();

    progState.startMillis = 0;
    progState.hitStreak = 0;
}

/**
 * Functions to interface with the button presses buffer.
 */

bool inPressesBuffer(int val)
{
    return inBuffer(bufButtonPresses, val);
}

bool isPressesBufferError()
{
    for (int i = 0; i < bufButtonPresses.size(); i++) {
        if (!inTargetsBuffer(bufButtonPresses[i])) {
            return true;
        }
    }

    return false;
}

bool isPressesBufferMatch()
{
    if (bufButtonTargets.size() != bufButtonPresses.size()) {
        return false;
    }

    for (int i = 0; i < bufButtonTargets.size(); i++) {
        if (!inPressesBuffer(bufButtonTargets[i])) {
            return false;
        }
    }

    return true;
}

/**
 * LED functions.
 */

void initLeds()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_BRIGHTNESS);
    ledStrip.show();
    ledStrip.clear();
}

void showErrorLedEffect()
{
    const int numIters = 3;
    const int delayMs = 250;
    const uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);

    for (int i = 0; i < numIters; i++) {
        ledStrip.fill(red);
        ledStrip.show();

        delay(delayMs);

        ledStrip.clear();
        ledStrip.show();

        delay(delayMs);
    }
}

uint32_t randomColor()
{
    return Adafruit_NeoPixel::Color(
        random(0, 255),
        random(0, 255),
        random(0, 255));
}

void showSuccessLedEffect(int numLoops = 1)
{
    const int delayMs = 250;

    numLoops = numLoops < 1 ? INT_MAX : numLoops;

    for (int k = 0; k < numLoops; k++) {
        ledStrip.clear();

        for (unsigned int i = 0; i < ledStrip.numPixels(); i++) {
            ledStrip.setPixelColor(i, randomColor());
            ledStrip.show();
            delay(delayMs);
        }
    }
}

void showTargetLeds()
{
    uint32_t color;
    bool colorExists;

    for (int i = 0; i < BUTTONS_NUM; i++) {
        colorExists = bufButtonColors.size() >= (i + 1);
        color = inPressesBuffer(i) || !colorExists ? 0 : bufButtonColors[i];
        ledStrip.setPixelColor(i, color);
    }

    ledStrip.show();
}

void refreshUnlockPhaseLeds()
{
    ledStrip.clear();

    uint32_t color;

    for (int i = 0; i < BUTTONS_NUM; i++) {
        color = UNLOCK_COLORS[progState.unlockColorIdx[i]];
        ledStrip.setPixelColor(i, color);
    }

    ledStrip.show();
}

/**
 * Functions to deal with game state progress.
 */

bool isExpired()
{
    if (progState.startMillis == 0) {
        return false;
    }

    unsigned long now = millis();

    if (now < progState.startMillis) {
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
    updateButtonColorsBuffer();
    showTargetLeds();
    progState.startMillis = millis();
    bufButtonPresses.clear();
}

void advanceProgress()
{
    progState.hitStreak++;

    int minHitStreak = getPhaseHitStreak(progState.currPhase);
    bool isNewPhase = progState.hitStreak >= minHitStreak;

    if (isNewPhase) {
        progState.hitStreak = 0;
        progState.currPhase++;
    }

    updateTargets();
}

bool hasFinished()
{
    return progState.currPhase >= FINAL_PHASE;
}

void errorAndRestart()
{
    showErrorLedEffect();
    resetStateToPhaseStart();
}

void onFinish()
{
    Serial.println(F("Game completed"));
    openRelay();
    showSuccessLedEffect(0);
}

bool isValidUnlockCombination()
{
    for (int i = 0; i < BUTTONS_NUM; i++) {
        if (progState.unlockColorIdx[i] != UNLOCK_COLORS_KEY[i]) {
            return false;
        }
    }

    return true;
}

void updateState()
{
    if (progState.isFinished) {
        return;
    }

    if (hasFinished()) {
        onFinish();
        progState.isFinished = true;
        return;
    }

    if (!progState.isUnlocked) {
        refreshUnlockPhaseLeds();

        if (isValidUnlockCombination()) {
            Serial.println(F("Valid unlock combination"));
            showSuccessLedEffect(2);
            progState.isUnlocked = true;
        }

        return;
    }

    if (progState.startMillis == 0) {
        Serial.println(F("First target update"));
        updateTargets();
    } else if (isPressesBufferError()) {
        Serial.println(F("Error: restart"));
        errorAndRestart();
    } else if (isExpired()) {
        Serial.println(F("Time expired: restart"));
        errorAndRestart();
    } else if (isPressesBufferMatch()) {
        Serial.println(F("OK: advancing progress"));
        advanceProgress();
    } else {
        showTargetLeds();
    }
}

/**
 * Button functions.
 */

void onUnlockPhasePress(int idx)
{
    Serial.print(F("## Knock::Unlock:"));
    Serial.println(idx);

    progState.unlockColorIdx[idx]++;
    progState.unlockColorIdx[idx] = progState.unlockColorIdx[idx] % UNLOCK_COLOR_NUM;
}

void onPress(int idx, int v, int up)
{
    if (!progState.isUnlocked) {
        onUnlockPhasePress(idx);
        return;
    }

    Serial.print(F("Press:"));
    Serial.println(idx);

    bool isDup = inBuffer(bufButtonPresses, idx);

    if (!isDup) {
        Serial.print(F("Pushing:"));
        Serial.println(idx);

        bufButtonPresses.push(idx);
    }
}

void initButtons()
{
    for (int i = 0; i < BUTTONS_NUM; i++) {
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

    initButtons();
    initState();
    initLeds();
    initRelay();
    initStateTimer();

    Serial.println(F(">> Starting compostin program"));
}

void loop()
{
    automaton.run();
}
