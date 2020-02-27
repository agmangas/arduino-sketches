#include "FeatherWingTFT35.h"

FeatherWingTFT35::FeatherWingTFT35()
    : sd()
    , tft(TFT_CS, TFT_DC)
    , reader(sd)
{
}

bool FeatherWingTFT35::begin(int flashMs)
{
    tft.begin();

    if (!sd.begin(SD_CS, SD_SCK_MHZ(25))) {
        return false;
    }

    if (flashMs > 0) {
        tft.fillScreen(HX8357_MAGENTA);
        delay(flashMs);
        tft.fillScreen(0);
    }

    return true;
}

bool FeatherWingTFT35::drawImage(String name, int16_t xOffset, int16_t yOffset)
{
    char buf[name.length() + 1];
    strcpy(buf, name.c_str());
    ImageReturnCode imgRet = reader.drawBMP(buf, tft, xOffset, yOffset);

    if (imgRet != IMAGE_SUCCESS) {
        reader.printStatus(imgRet);
    }

    return imgRet == IMAGE_SUCCESS;
}

String FeatherWingTFT35::frameName(String frameId, int frameNum)
{
    const int numBufSize = 16;
    char numBuf[numBufSize];
    sprintf(numBuf, "%02d", frameNum);
    String name = String("/" + frameId + "-" + numBuf + ".bmp");

    return name;
}

int FeatherWingTFT35::findSequenceLen(String frameId)
{
    String name;
    int currLen = -1;

    do {
        currLen++;
        name = frameName(frameId, currLen);
    } while (sd.exists(name.c_str()));

    return currLen;
}

bool FeatherWingTFT35::drawSequence(String frameId, uint16_t clearColor)
{
    int num = findSequenceLen(frameId);

    if (num <= 0) {
        Serial.print(F("Sequence length == 0: "));
        Serial.println(frameId);
        return false;
    }

    String firstName = frameName(frameId, 0);
    char firstNameBuf[firstName.length() + 1];
    strcpy(firstNameBuf, firstName.c_str());

    int32_t width;
    int32_t height;
    ImageReturnCode imgRet;

    imgRet = reader.bmpDimensions(firstNameBuf, &width, &height);

    if (imgRet != IMAGE_SUCCESS) {
        reader.printStatus(imgRet);
        return false;
    }

    if (clearColor >= 0) {
        tft.fillScreen(clearColor);
    }

    int16_t xOffset = width <= TFT35_WIDTH
        ? floor((double)(TFT35_WIDTH - width) / 2.0)
        : 0;

    int16_t yOffset = height <= TFT35_HEIGHT
        ? floor((double)(TFT35_HEIGHT - height) / 2.0)
        : 0;

    for (int i = 0; i < num; i++) {
        if (!drawImage(frameName(frameId, i), xOffset, yOffset)) {
            return false;
        }
    }

    if (clearColor >= 0) {
        tft.fillScreen(clearColor);
    }

    return true;
}
