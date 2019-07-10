#include "rdm630.h"
#include <Automaton.h>

/**
 * RFID reader.
 */

const byte PIN_RFID_RX = 4;
const byte PIN_RFID_TX = 5;

RDM6300 rfidReader(PIN_RFID_RX, PIN_RFID_TX);

const byte NUM_VALID_TAGS = 2;

String validTags[NUM_VALID_TAGS] = {
    "1D00277B1300",
    "1D00277B1300"};

Atm_timer rfidTimer;

const int RFID_TIMER_MS = 5000;

/**
 * Output bits.
 */

Atm_led ledRfid;

const int OUTPUT_PIN_RFID = 6;
const int OUTPUT_MS_DURATION = 200;
const int OUTPUT_MS_PAUSE_DURATION = 50;
const bool OUTPUT_ACTIVE_LOW = true;

/**
 * RFID functions.
 */

void onRfidTimer(int idx, int v, int up)
{
    pollRfidReader();
}

void initRfid()
{
    rfidReader.begin();

    rfidTimer
        .begin(RFID_TIMER_MS)
        .repeat(-1)
        .onTimer(onRfidTimer)
        .start();
}

void pollRfidReader()
{
    String tagId;

    Serial.println(F("Reading RFID"));

    tagId = rfidReader.getTagId();

    if (!tagId.length())
    {
        return;
    }

    Serial.print(F("Tag :: "));
    Serial.println(tagId);

    for (int i = 0; i < NUM_VALID_TAGS; i++)
    {
        if (validTags[i].compareTo(tagId) == 0)
        {
            Serial.println(F("RFID :: Pulse"));
            ledRfid.start();
            return;
        }
    }
}

/**
 * Output bit functions.
 */

void initOutputs()
{
    ledRfid
        .begin(OUTPUT_PIN_RFID, OUTPUT_ACTIVE_LOW)
        .blink(OUTPUT_MS_DURATION, OUTPUT_MS_PAUSE_DURATION)
        .repeat(1);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initRfid();
    initOutputs();

    Serial.println(F(">> Starting Runebook sensors program"));
}

void loop()
{
    automaton.run();
}
