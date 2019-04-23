#include <Automaton.h>

/**
 * Program state.
 */

typedef struct programState
{
    bool isPotsOk;
} ProgramState;

ProgramState progState = {
    .isPotsOk = false};

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
    2, 1, 0, 0, 2};

const unsigned long POTS_BOUNCE_MS = 1000;

/**
   Potentiometer functions
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

void updateState()
{
    if (progState.isPotsOk || !isValidPotsCombination())
    {
        return;
    }

    Serial.print(F("Valid pots: Checking in "));
    Serial.print(POTS_BOUNCE_MS);
    Serial.println(F(" ms"));

    delay(POTS_BOUNCE_MS);

    if (isValidPotsCombination())
    {
        Serial.println(F("Pots OK"));
        progState.isPotsOk = true;
        openRelay();
    }
    else
    {
        Serial.println(F("Pots changed during bounce delay"));
    }
}

void setup()
{
    Serial.begin(9600);

    initPots();
    initRelay();

    Serial.println(F(">> Starting quidditch puzzle program"));
}

void loop()
{
    automaton.run();
    updateState();
}
