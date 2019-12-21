#include <SerialRFID.h>
#include <SoftwareSerial.h>

/**
 * RFID readers.
 */

const uint8_t RFID_NUM = 3;
const uint8_t RFID_PINS_RX[RFID_NUM] = { 2, 4, 6 };
const uint8_t RFID_PINS_TX[RFID_NUM] = { 3, 5, 7 };

SoftwareSerial rfidSoftSerials[RFID_NUM] = {
    SoftwareSerial(RFID_PINS_RX[0], RFID_PINS_TX[0]),
    SoftwareSerial(RFID_PINS_RX[1], RFID_PINS_TX[1]),
    SoftwareSerial(RFID_PINS_RX[2], RFID_PINS_TX[2])
};

SerialRFID rfids[RFID_NUM] = {
    SerialRFID(rfidSoftSerials[0]),
    SerialRFID(rfidSoftSerials[1]),
    SerialRFID(rfidSoftSerials[2])
};

const uint8_t RFID_VALID_OPTIONS = 2;

char keyTags[RFID_NUM][RFID_VALID_OPTIONS][SIZE_TAG_ID] = {
    { "5C00CADB5A19", "5C00CADB5A19" },
    { "5C00CADB5A19", "5C00CADB5A19" },
    { "5C00CADB5A19", "5C00CADB5A19" }
};

/**
 * Program state.
 */

char tagBuffer[SIZE_TAG_ID];
bool rfidsUnlocked[RFID_NUM];

typedef struct programState {
    bool* rfidsUnlocked;
} ProgramState;

ProgramState progState;

void initState()
{
    for (int i = 0; i < RFID_NUM; i++) {
        progState.rfidsUnlocked[i] = false;
    }
}

/**
 * RFID functions.
 */

void initRfids()
{
    for (int i = 0; i < RFID_NUM; i++) {
        rfidSoftSerials[i].begin(9600);
    }

    rfidSoftSerials[0].listen();
    Serial.print(F("Listening on #0"));
}

uint8_t activeRfidIndex()
{
    for (int i = 0; i < RFID_NUM; i++) {
        if (progState.rfidsUnlocked[i] == false) {
            return i;
        }
    }

    return 0;
}

void onValidTag(uint8_t readerIdx)
{
    if (progState.rfidsUnlocked[readerIdx]) {
        return;
    }

    Serial.print(F("Valid tag on #"));
    Serial.println(readerIdx);

    progState.rfidsUnlocked[readerIdx] = true;
}

void rfidLoop()
{
    uint8_t activeIdx = activeRfidIndex();

    bool isReplaced = rfidSoftSerials[activeIdx].listen();

    if (isReplaced) {
        Serial.print(F("Listening on #"));
        Serial.println(activeIdx);
    }

    bool someTag = rfids[activeIdx].readTag(
        tagBuffer,
        sizeof(tagBuffer));

    if (!someTag) {
        return;
    }

    Serial.print("Tag: ");
    Serial.print(tagBuffer);

    bool isValidTag = false;

    for (int opt = 0; opt < RFID_VALID_OPTIONS; opt++) {
        isValidTag = SerialRFID::isEqualTag(
            tagBuffer,
            keyTags[activeIdx][opt]);

        if (isValidTag) {
            break;
        }
    }

    if (isValidTag) {
        onValidTag(activeIdx);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initRfids();

    Serial.println(F(">> Starting zephyr"));
}

void loop()
{
    rfidLoop();
}