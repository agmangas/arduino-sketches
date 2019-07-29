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
 * Countdown timer.
 */

Atm_timer timerCountdown;

const int TIMER_COUNTDOWN_MS = 5000;

/**
 * Program state.
 */

int currChoices[PLAYERS_NUM];

typedef struct programState
{
    int *currChoices;
} ProgramState;

ProgramState progState = {
    .currChoices = currChoices};

void initState()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        progState.currChoices[p] = 0;
    }
}

/**
 * Player LED strips.
 */

const int LED_PLAYER_BRIGHTNESS = 200;

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
 * Player LED functions.
 */

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
 * Countdown timer functions.
 */

bool isTimerCountdownIdle()
{
    return timerCountdown.state() == Atm_timer::IDLE;
}

void onTimerCountdown(int idx, int v, int up)
{
    Serial.println(F("Countdown timer"));
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
        timerCountdown.start();
    }
    else
    {
        Serial.println(F("Countdown timer is not idle"));
    }
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

void initButtons()
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
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initButtons();
    initPlayerLeds();
    initTimerCountdown();

    Serial.println(F(">> Starting quiz program"));
}

void loop()
{
    automaton.run();
}