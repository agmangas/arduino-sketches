#include <Automaton.h>

/**
 * Program state.
 */

typedef struct programState
{
    bool isPotsOk;
    unsigned long counterOk;
    unsigned long counterErr;
} ProgramState;

ProgramState progState = {
    .isPotsOk = false,
    .counterOk = 0,
    .counterErr = 0};

const int TIMER_STATE_MS = 500;
const int MAX_COUNTER = 30;
const int COUNTER_OK_THRESHOLD = 3;
const int COUNTER_ERR_THRESHOLD = 10;

Atm_timer timerState;

/**
 * Relay.
 */

const int PIN_RELAY = 10;

/**
 * Potentiometers.
 */

const int POTS_NUM = 5;

int potPins[POTS_NUM] = {
    A0, A1, A2, A3, A4};

Atm_analog pots[POTS_NUM];

const byte POTS_RANGE_LO = 0;
const byte POTS_RANGE_HI = 2;

byte potsKey[POTS_NUM] = {
    2, 0, 0, 1, 2};

const unsigned long POTS_BOUNCE_MS = 1000;

/**
 * Timer functions.
 */

void onStateTimer(int idx, int v, int up)
{
    bool isValid = isValidPotsCombination();

    if (isValid)
    {
        if (progState.counterOk < MAX_COUNTER)
        {
            Serial.println(F("Counter OK ++"));
            progState.counterOk++;
        }

        progState.counterErr = 0;
    }
    else
    {
        if (progState.counterErr < MAX_COUNTER)
        {
            Serial.println(F("Counter Error ++"));
            progState.counterErr++;
        }

        progState.counterOk = 0;
    }

    if (progState.counterOk >= COUNTER_OK_THRESHOLD &&
        progState.isPotsOk == false)
    {
        progState.isPotsOk = true;
        openRelay();
    }
}

void initStateTimer()
{
    timerState
        .begin(TIMER_STATE_MS)
        .repeat(-1)
        .onTimer(onStateTimer)
        .start();
}

/**
 * Potentiometer functions.
 */

void onPotChange(int idx, int v, int up)
{
    Serial.print(F("Pot:"));
    Serial.print(idx);
    Serial.print(F(":"));
    Serial.println(v);
}

bool isValidPotsCombination()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        if (potsKey[i] != pots[i].state())
        {
            return false;
        }
    }

    return true;
}

void initPots()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        pots[i]
            .begin(potPins[i])
            .range(POTS_RANGE_LO, POTS_RANGE_HI)
            .onChange(onPotChange, i);
    }
}

/**
 * Relay functions.
 */

void lockRelay()
{
    if (digitalRead(PIN_RELAY) == HIGH)
    {
        Serial.println(F("Relay:Lock"));
    }

    digitalWrite(PIN_RELAY, LOW);
}

void openRelay()
{
    if (digitalRead(PIN_RELAY) == LOW)
    {
        Serial.println(F("Relay:Open"));
    }

    digitalWrite(PIN_RELAY, HIGH);
}

void initRelay()
{
    pinMode(PIN_RELAY, OUTPUT);
    lockRelay();
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initPots();
    initRelay();
    initStateTimer();

    Serial.println(F(">> Starting quidditch puzzle program"));
}

void loop()
{
    automaton.run();
}
