#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Player buttons.
 */

const int PLAYERS_NUM = 3;
const int OPTIONS_NUM = 2;

const int BUTTONS_PINS[PLAYERS_NUM][OPTIONS_NUM] = {
    {2, 3},
    {4, 5},
    {6, 7}};

const int BUTTONS_LEDS_PINS[PLAYERS_NUM][OPTIONS_NUM] = {
    {A0, A1},
    {A2, A3},
    {A4, A5}};

Atm_button buttons[PLAYERS_NUM][OPTIONS_NUM];
Atm_led buttonsLeds[PLAYERS_NUM][OPTIONS_NUM];

/**
 * Show host button.
 */

const int HOST_BUTTON_PIN = A6;
Atm_button buttonHost;

/**
 * Countdown timer.
 */

Atm_timer timerCountdown;

const int TIMER_COUNTDOWN_MS = 5000;

/**
 * Countdown LED timer.
 */

Atm_timer timerLedCountdown;

const int TIMER_LED_COUNTDOWN_MS = 150;

/**
 * Colors.
 */

const uint32_t COLOR_CORRECT = Adafruit_NeoPixel::Color(0, 255, 0);
const uint32_t COLOR_ERROR = Adafruit_NeoPixel::Color(255, 0, 0);

/**
 * Player LED strips.
 */

const int LED_PLAYER_BRIGHTNESS = 150;

const int LED_PLAYER_PINS[PLAYERS_NUM] = {
    8, 9, 10};

const int LED_PLAYER_NUM[PLAYERS_NUM] = {
    10, 10, 10};

Adafruit_NeoPixel ledPlayerStrips[PLAYERS_NUM] = {
    Adafruit_NeoPixel(
        LED_PLAYER_NUM[0],
        LED_PLAYER_PINS[0],
        NEO_RGB + NEO_KHZ800),
    Adafruit_NeoPixel(
        LED_PLAYER_NUM[1],
        LED_PLAYER_PINS[1],
        NEO_RGB + NEO_KHZ800),
    Adafruit_NeoPixel(
        LED_PLAYER_NUM[2],
        LED_PLAYER_PINS[2],
        NEO_RGB + NEO_KHZ800)};

/**
 * Countdown LED strip.
 */

const int LED_COUNTDOWN_BRIGHTNESS = 220;
const int LED_COUNTDOWN_PIN = 11;
const int LED_COUNTDOWN_NUM = 10;

Adafruit_NeoPixel ledCountdown = Adafruit_NeoPixel(
    LED_COUNTDOWN_NUM,
    LED_COUNTDOWN_PIN,
    NEO_RGB + NEO_KHZ800);

/**
 * Global scoreboard strip.
 */

const int LED_GLOBAL_BRIGHTNESS = 220;
const int LED_GLOBAL_PIN = 12;
const int LED_GLOBAL_NUM = 30;

Adafruit_NeoPixel ledGlobal = Adafruit_NeoPixel(
    LED_GLOBAL_NUM,
    LED_GLOBAL_PIN,
    NEO_RGB + NEO_KHZ800);

/**
 * Solution key.
 */

const int NUM_PHASES = 10;

const int SOLUTION_KEY[NUM_PHASES] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

/**
 * Program state.
 */

typedef struct programState
{
    int currChoices[PLAYERS_NUM];
    bool phaseResults[PLAYERS_NUM][NUM_PHASES];
    int currPhase;
    unsigned long countdownStartMillis;
} ProgramState;

ProgramState progState = {
    .currChoices = {0},
    .phaseResults = {{false}},
    .currPhase = 0,
    .countdownStartMillis = 0};

void initState()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        for (int f = 0; f < NUM_PHASES; f++)
        {
            progState.phaseResults[p][f] = false;
        }

        progState.currChoices[p] = 0;
    }

    progState.currPhase = 0;
    progState.countdownStartMillis = 0;
}

bool isFinished()
{
    return progState.currPhase >= NUM_PHASES;
}

void resetProgram()
{
    Serial.println(F("Program reset"));

    initState();

    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        ledPlayerStrips[p].clear();
        ledPlayerStrips[p].show();
    }

    ledCountdown.clear();
    ledCountdown.show();

    ledGlobal.clear();
    ledGlobal.show();
}

/**
 * Player LED functions.
 */

void showPlayerLedCorrect(int pIdx)
{
    ledPlayerStrips[pIdx].fill(COLOR_CORRECT);
    ledPlayerStrips[pIdx].show();
}

void showPlayerLedError(int pIdx)
{
    ledPlayerStrips[pIdx].fill(COLOR_ERROR);
    ledPlayerStrips[pIdx].show();
}

void clearPlayerLeds()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        ledPlayerStrips[p].clear();
        ledPlayerStrips[p].show();
    }
}

void initPlayerLeds()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        ledPlayerStrips[p].begin();
        ledPlayerStrips[p].setBrightness(LED_PLAYER_BRIGHTNESS);
        ledPlayerStrips[p].clear();
        ledPlayerStrips[p].show();
    }
}

/**
 * Countdown LED functions.
 */

void initCountdownLed()
{
    ledCountdown.begin();
    ledCountdown.setBrightness(LED_COUNTDOWN_BRIGHTNESS);
    ledCountdown.clear();
    ledCountdown.show();
}

/**
 * Global scoreboard LED strip functions.
 */

void showGlobalLed()
{
    ledGlobal.clear();

    int ledsPerPlayer = floor(((float)LED_GLOBAL_NUM) / ((float)PLAYERS_NUM));
    int ledsPerPhase = floor(((float)ledsPerPlayer) / ((float)NUM_PHASES));

    if (ledsPerPhase <= 0)
    {
        Serial.println(F("Warning :: Invalid number of global LEDs"));
        return;
    }

    int playerOffset;
    int phaseOffset;
    int currLedIdx;
    uint32_t color;

    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        playerOffset = p * ledsPerPlayer;

        for (int f = 0; f < progState.currPhase; f++)
        {
            phaseOffset = f * ledsPerPhase;

            color = progState.phaseResults[p][f]
                        ? COLOR_CORRECT
                        : COLOR_ERROR;

            for (int i = 0; i < ledsPerPhase; i++)
            {
                currLedIdx = playerOffset + phaseOffset + i;

                if (currLedIdx >= (playerOffset + ledsPerPlayer))
                {
                    Serial.println(F("Warning :: Global LED overflow"));
                }

                ledGlobal.setPixelColor(currLedIdx, color);
            }
        }
    }

    ledGlobal.show();
}

void initGlobalLed()
{
    ledGlobal.begin();
    ledGlobal.setBrightness(LED_GLOBAL_BRIGHTNESS);
    ledGlobal.clear();
    ledGlobal.show();
}

/**
 * Countdown timer functions.
 */

bool isTimerCountdownIdle()
{
    return timerCountdown.state() == Atm_timer::IDLE;
}

void blockAndWaitForReset()
{
    const unsigned long iterDelayMs = 5000;

    while (true)
    {
        Serial.println(F("The end"));

        delay(iterDelayMs);

        if (digitalRead(HOST_BUTTON_PIN) == LOW)
        {
            resetProgram();
            break;
        }
    }
}

void onTimerCountdown(int idx, int v, int up)
{
    Serial.println(F("Countdown timer finished"));

    if (isFinished())
    {
        Serial.println(F("Ignoring countdown timer :: Final phase"));
        return;
    }

    int validChoice = SOLUTION_KEY[progState.currPhase];
    bool isValidChoice;

    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        isValidChoice = progState.currChoices[p] == validChoice;
        progState.phaseResults[p][progState.currPhase] = isValidChoice;

        Serial.print(F("Player #"));
        Serial.print(p);
        Serial.print(F(" :: Phase #"));
        Serial.print(progState.currPhase);

        if (isValidChoice)
        {
            Serial.println(F(" :: OK"));
            showPlayerLedCorrect(p);
        }
        else
        {
            Serial.println(F(" :: Error"));
            showPlayerLedError(p);
        }
    }

    progState.currPhase++;
    progState.countdownStartMillis = 0;
    showGlobalLed();

    if (isFinished())
    {
        blockAndWaitForReset();
    }
}

void initTimerCountdown()
{
    timerCountdown
        .begin(TIMER_COUNTDOWN_MS)
        .repeat(1)
        .onTimer(onTimerCountdown);
}

void startTimerCountdown()
{
    if (isTimerCountdownIdle())
    {
        Serial.println(F("Starting countdown timer"));
        clearPlayerLeds();
        progState.countdownStartMillis = millis();
        timerCountdown.start();
    }
    else
    {
        Serial.println(F("Countdown timer is not idle"));
    }
}

void onTimerLedCountdown(int idx, int v, int up)
{
    ledCountdown.clear();

    if (progState.countdownStartMillis == 0)
    {
        ledCountdown.show();
        return;
    }

    unsigned long now = millis();
    unsigned long endMillis = progState.countdownStartMillis + TIMER_COUNTDOWN_MS;

    if (now >= endMillis)
    {
        ledCountdown.show();
        return;
    }

    unsigned long diff = endMillis - now;
    float ratio = ((float)diff) / ((float)TIMER_COUNTDOWN_MS);
    ratio = ratio > 1.0 ? 1.0 : ratio;
    int numLedsOn = floor(LED_COUNTDOWN_NUM * ratio);

    for (int i = 0; i < numLedsOn; i++)
    {
        ledCountdown.setPixelColor(i, COLOR_CORRECT);
    }

    ledCountdown.show();
}

void initTimerLedCountdown()
{
    timerLedCountdown
        .begin(TIMER_LED_COUNTDOWN_MS)
        .repeat(-1)
        .onTimer(onTimerLedCountdown)
        .start();
}

/**
 * Player button functions.
 */

int flattenButtonIndex(int plyIdx, int optIdx)
{
    return optIdx + plyIdx * OPTIONS_NUM;
}

int flatToPlayerIndex(int flatIdx)
{
    return floor(((float)flatIdx) / ((float)OPTIONS_NUM));
}

int flatToOptionIndex(int flatIdx)
{
    return flatIdx % OPTIONS_NUM;
}

void onPlayerButton(int idx, int v, int up)
{
    int plyIdx = flatToPlayerIndex(idx);
    int optIdx = flatToOptionIndex(idx);

    Serial.print(F("Button::P"));
    Serial.print(plyIdx);
    Serial.print(F("::O"));
    Serial.println(optIdx);

    for (int i = 0; i < OPTIONS_NUM; i++)
    {
        buttonsLeds[plyIdx][i].trigger(
            (i == optIdx) ? Atm_led::EVT_ON : Atm_led::EVT_OFF);
    }

    progState.currChoices[plyIdx] = optIdx;
}

void initPlayerButtons()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        for (int o = 0; o < OPTIONS_NUM; o++)
        {
            buttonsLeds[p][o]
                .begin(BUTTONS_LEDS_PINS[p][o])
                .trigger(Atm_led::EVT_OFF);

            buttons[p][o]
                .begin(BUTTONS_PINS[p][o])
                .onPress(onPlayerButton, flattenButtonIndex(p, o));
        }
    }
}

/**
 * Show host button functions.
 */

void onHostButton(int idx, int v, int up)
{
    Serial.println(F("Host button"));
    startTimerCountdown();
}

void initHostButton()
{
    buttonHost
        .begin(HOST_BUTTON_PIN)
        .onPress(onHostButton);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initPlayerButtons();
    initHostButton();
    initPlayerLeds();
    initTimerCountdown();
    initCountdownLed();
    initGlobalLed();
    initTimerLedCountdown();

    Serial.println(F(">> Starting quiz program"));
}

void loop()
{
    automaton.run();
}