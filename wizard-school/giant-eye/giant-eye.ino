#include "rdm630.h"
#include <Automaton.h>

/**
 * Relays.
 */

const int PIN_RELAY_LDR = 10;
const int PIN_RELAY_RFID = 11;

/**
 * LDR.
 */

const int PIN_BUTTON_LDR = 5;
const int LDR_DEBOUNCE_MS = 250;
Atm_button buttonLdr;

/**
 * RFID modules.
 */

const int NUM_READERS = 1;

const int RFID_PIN_RX = 2;
const int RFID_PIN_TX = 3;

RDM6300 rfid01(RFID_PIN_RX, RFID_PIN_TX);

RDM6300 rfidReaders[NUM_READERS] = {
    rfid01};

const unsigned int EMPTY_TOLERANCE = 1;

String currentTags[NUM_READERS];

String validTags[NUM_READERS] = {
    "1D0027B6BE00"};

/**
 * Program state.
 */

typedef struct programState
{
    bool isRelayOpenRfid;
    bool isRelayOpenLdr;
    bool isLdrPressed;
    unsigned int emptyReadCount[NUM_READERS];
} ProgramState;

ProgramState progState = {
    .isRelayOpenRfid = false,
    .isRelayOpenLdr = false,
    .isLdrPressed = false,
    .emptyReadCount = {0}};

/**
 * RFID modules functions.
 */

void initRfidReaders()
{
    for (int i = 0; i < NUM_READERS; i++)
    {
        rfidReaders[i].begin();
    }
}

void pollRfidReaders()
{
    String tagId;

    for (int i = 0; i < NUM_READERS; i++)
    {
        tagId = rfidReaders[i].getTagId();

        if (tagId.length())
        {
            progState.emptyReadCount[i] = 0;
        }
        else if (progState.emptyReadCount[i] <= EMPTY_TOLERANCE)
        {
            progState.emptyReadCount[i] += 1;
        }

        if (!tagId.length() &&
            currentTags[i].length() &&
            progState.emptyReadCount[i] <= EMPTY_TOLERANCE)
        {
            Serial.print(F("Ignoring empty read on #"));
            Serial.println(i);
            return;
        }

        currentTags[i] = tagId;
    }
}

bool isTagDefined(int idx)
{
    return currentTags[idx].length() > 0;
}

bool areCurrentTagsValid()
{
    for (int i = 0; i < NUM_READERS; i++)
    {
        if (!isTagDefined(i))
        {
            return false;
        }

        if (validTags[i].compareTo(currentTags[i]) != 0)
        {
            return false;
        }
    }

    return true;
}

void printCurrentTags()
{
    Serial.print(F("Tags::"));
    Serial.println(millis());

    for (int i = 0; i < NUM_READERS; i++)
    {
        Serial.print(i);
        Serial.print(F("::"));
        Serial.println(currentTags[i]);
    }
}

void updateRelayRfid()
{
    if (areCurrentTagsValid() == true &&
        progState.isRelayOpenRfid == false)
    {
        Serial.println(F("Opening relay"));
        openRelayRfid();
    }
    else if (areCurrentTagsValid() == false &&
             progState.isRelayOpenRfid == true)
    {
        Serial.println(F("Locking relay"));
        lockRelayRfid();
    }
}

/**
 * Relay functions.
 */

void lockRelayRfid()
{
    digitalWrite(PIN_RELAY_RFID, LOW);
    progState.isRelayOpenRfid = false;
}

void openRelayRfid()
{
    digitalWrite(PIN_RELAY_RFID, HIGH);
    progState.isRelayOpenRfid = true;
}

void lockRelayLdr()
{
    digitalWrite(PIN_RELAY_LDR, LOW);
    progState.isRelayOpenLdr = false;
}

void openRelayLdr()
{
    digitalWrite(PIN_RELAY_LDR, HIGH);
    progState.isRelayOpenLdr = true;
}

void initRelays()
{
    pinMode(PIN_RELAY_RFID, OUTPUT);
    pinMode(PIN_RELAY_LDR, OUTPUT);

    lockRelayRfid();
    lockRelayLdr();
}

/**
 * LDR functions.
 */

void onLdrPress(int idx, int v, int up)
{
    Serial.println(F("LDR press"));
    progState.isLdrPressed = true;
}

void onLdrRelease(int idx, int v, int up)
{
    Serial.print(F("LDR release"));
    progState.isLdrPressed = false;
}

void initButtonLdr()
{
    buttonLdr
        .begin(PIN_BUTTON_LDR)
        .debounce(LDR_DEBOUNCE_MS)
        .onPress(onLdrPress)
        .onRelease(onLdrRelease);
}

void updateRelayLdr()
{
    if (progState.isLdrPressed == true &&
        progState.isRelayOpenRfid == false &&
        progState.isRelayOpenLdr == false)
    {
        Serial.println(F("Opening LDR relay"));
        openRelayLdr();
    }
    else if (progState.isRelayOpenLdr == true)
    {
        Serial.println(F("Locking LDR relay"));
        lockRelayLdr();
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initRfidReaders();
    initRelays();
    initButtonLdr();

    Serial.println(F(">> Starting giant's eye program"));
}

void loop()
{
    pollRfidReaders();
    printCurrentTags();
    updateRelayRfid();
    updateRelayLdr();
    automaton.run();
}
