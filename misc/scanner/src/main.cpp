#include <FeatherWingTFT35.h>
#include <SerialRFID.h>

FeatherWingTFT35 tftWing;
SerialRFID rfid(Serial1);
char tagBuf[SIZE_TAG_ID];
const int NUM_TAGS = 3;

char tagIds[NUM_TAGS][SIZE_TAG_ID] = {
    "5C00CADB5A17",
    "5C00CADB5A18",
    "5C00CADB5A19"
};

String sequenceNames[NUM_TAGS] = {
    String("sequence1"),
    String("sequence2"),
    String("sequence3")
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

    Serial.print(F("Drawing sequence #"));
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

    tftWing.tft.fillScreen(HX8357_CYAN);
}

void loop()
{
    drawTagSequence();
}
