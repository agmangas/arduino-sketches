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
    "0C0031B18500",
    "020001D82D00",
    "62003BBF2100",
    "0200017C3400"
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

bool allTagsOk()
{
    for (uint8_t i = 0; i < NUM_READERS; i++) {
        if (currentTags[i].length() == 0
            || validTags[i].compareTo(currentTags[i]) != 0) {
            return false;
        }
    }

    return true;
}

void refreshLasers()
{
    bool isOk = allTagsOk();

    for (uint8_t i = 0; i < NUM_READERS; i++) {
        digitalWrite(LASER_PINS[i], isOk ? HIGH : LOW);
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