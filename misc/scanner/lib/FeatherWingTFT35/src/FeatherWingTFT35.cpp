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

int FeatherWingTFT35::findNumFrames(String frameId)
{
    String name;
    int ret = -1;

    do {
        ret++;
        name = frameName(frameId, ret);
    } while (sd.exists(name.c_str()));

    return ret;
}

bool FeatherWingTFT35::drawFrames(String frameId)
{
    int num = findNumFrames(frameId);

    if (num <= 0) {
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
        return false;
    }

    int16_t xOffset = floor((double)(TFT35_WIDTH - width) / 2.0);
    int16_t yOffset = floor((double)(TFT35_HEIGHT - height) / 2.0);

    for (int i = 0; i < num; i++) {
        if (!drawImage(frameName(frameId, i), xOffset, yOffset)) {
            return false;
        }
    }

    return true;
}
