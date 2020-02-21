#include "FeatherWingTFT35.h"

FeatherWingTFT35 tftWing;

void setup(void)
{
    Serial.begin(9600);

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
    tftWing.drawFrames("frame");
}
