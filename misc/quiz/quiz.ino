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

Atm_button buttons[PLAYERS_NUM][OPTIONS_NUM];

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

void clearPlayerButtonLed(int plyIdx, int optIdx)
{
}

void showPlayerButtonLed(int plyIdx, int optIdx)
{
}

void refreshPlayerButtonLeds()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        for (int o = 0; o < OPTIONS_NUM; o++)
        {
            clearPlayerButtonLed(p, o);
        }

        showPlayerButtonLed(p, progState.currChoices[p]);
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

    progState.currChoices[plyIdx] = optIdx;
}

void initButtons()
{
    for (int p = 0; p < PLAYERS_NUM; p++)
    {
        for (int o = 0; o < OPTIONS_NUM; o++)
        {
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

    Serial.println(F(">> Starting quiz program"));
}

void loop()
{
    automaton.run();
}