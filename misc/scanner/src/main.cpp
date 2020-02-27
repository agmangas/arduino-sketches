#include <FeatherWingTFT35.h>
#include <SerialRFID.h>

FeatherWingTFT35 tftWing;
SerialRFID rfid(Serial1);
char tagBuf[SIZE_TAG_ID];
const int NUM_TAGS = 3;

char tagIds[NUM_TAGS][SIZE_TAG_ID] = {
    "02000CF612EA",
    "02000D18495E",
    "020001607417"
};

String sequenceNames[NUM_TAGS] = {
    String("frame"),
    String("seq01"),
    String("seq02")
};

int getCurrentTagIndex()
{
    if (!rfid.readTag(tagBuf, sizeof(tagBuf))) {
        return -1;
    }

    Serial.print(F("Tag: "));
    Serial.println(tagBuf);

    for (int i = 0; i < NUM_TAGS; i++) {
        if (SerialRFID::isEqualTag(tagBuf, tagIds[i])) {
            Serial.print(F("Match: #"));
            Serial.println(i);
            return i;
        }
    }

    return -1;
}

void drawTagSequence()
{
    int tagIdx = getCurrentTagIndex();

    if (tagIdx < 0) {
        return;
    }

    Serial.print(F("Drawing sequence "));
    Serial.println(sequenceNames[tagIdx]);

    tftWing.drawSequence(sequenceNames[tagIdx]);
}

void setup(void)
{
    Serial.begin(9600);
    Serial1.begin(9600);

    if (!tftWing.begin(1000)) {
        Serial.println(F("Failed initializing TFT"));
        while (true) {
            delay(100);
        }
    }

    tftWing.tft.fillScreen(0);

    Serial.println(F(">> Starting scanner"));
}

void loop()
{
    drawTagSequence();
}
