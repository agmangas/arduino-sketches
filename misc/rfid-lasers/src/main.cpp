#include <rdm630.h>

const uint8_t NUM_READERS = 4;

// RX, TX
RDM6300 rfid01(2, 3);
RDM6300 rfid02(4, 5);
RDM6300 rfid03(6, 7);
RDM6300 rfid04(8, 9);

RDM6300 rfidReaders[NUM_READERS] = {
    rfid01,
    rfid02,
    rfid03,
    rfid04
};

String currentTags[NUM_READERS];

String validTags[NUM_READERS] = {
    "1D00278F8200",
    "1D00277EC900",
    "1D00278E7600",
    "1D0027AA9B00"
};

const uint8_t LASER_PINS[NUM_READERS] = {
    10, 11, 12, A0
};

void initRfid()
{
    for (uint8_t i = 0; i < NUM_READERS; i++) {
        rfidReaders[i].begin();
    }
}

void initLasers()
{
    for (uint8_t i = 0; i < NUM_READERS; i++) {
        pinMode(LASER_PINS[i], OUTPUT);
    }
}

void pollRfid()
{
    String tagId;

    for (uint8_t i = 0; i < NUM_READERS; i++) {
        tagId = rfidReaders[i].getTagId();

        Serial.print(F("RFID #"));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.println(tagId.length() ? tagId : F("--"));

        currentTags[i] = tagId;
    }
}

void refreshLasers()
{
    bool isDefined;
    bool isValid;

    for (uint8_t i = 0; i < NUM_READERS; i++) {
        isDefined = currentTags[i].length() > 0;
        isValid = validTags[i].compareTo(currentTags[i]) == 0;

        digitalWrite(
            LASER_PINS[i],
            isDefined && isValid ? HIGH : LOW);
    }
}

void setup()
{
    Serial.begin(9600);

    initLasers();
    initRfid();

    Serial.println(">> Starting program");
}

void loop()
{
    pollRfid();
    refreshLasers();
}