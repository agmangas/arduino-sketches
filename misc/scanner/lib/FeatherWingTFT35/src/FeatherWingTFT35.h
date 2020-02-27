#ifndef _FEATHER_WING_TFT35_
#define _FEATHER_WING_TFT35_

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_HX8357.h>
#include <Adafruit_ImageReader.h>
#include <Adafruit_SPIFlash.h>
#include <SdFat.h>

#ifdef ESP8266
#define STMPE_CS 16
#define TFT_CS 0
#define TFT_DC 15
#define SD_CS 2
#endif
#ifdef ESP32
#define STMPE_CS 32
#define TFT_CS 15
#define TFT_DC 33
#define SD_CS 14
#endif
#ifdef TEENSYDUINO
#define TFT_DC 10
#define TFT_CS 4
#define STMPE_CS 3
#define SD_CS 8
#endif
#ifdef ARDUINO_STM32_FEATHER
#define TFT_DC PB4
#define TFT_CS PA15
#define STMPE_CS PC7
#define SD_CS PC5
#endif
#ifdef ARDUINO_NRF52832_FEATHER
#define STMPE_CS 30
#define TFT_CS 13
#define TFT_DC 11
#define SD_CS 27
#endif
#if defined(ARDUINO_MAX32620FTHR) || defined(ARDUINO_MAX32630FTHR)
#define TFT_DC P5_4
#define TFT_CS P5_3
#define STMPE_CS P3_3
#define SD_CS P3_2
#endif
#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_FEATHER_M0) || defined(__AVR_ATmega328P__) || defined(ARDUINO_SAMD_ZERO) || defined(__SAMD51__) || defined(__SAM3X8E__) || defined(ARDUINO_NRF52840_FEATHER)
#define STMPE_CS 6
#define TFT_CS 9
#define TFT_DC 10
#define SD_CS 5
#endif

class FeatherWingTFT35 {
public:
    static const int32_t TFT35_WIDTH = 320;
    static const int32_t TFT35_HEIGHT = 480;

    FeatherWingTFT35();
    bool begin(int = -1);
    bool drawImage(String, int16_t = 0, int16_t = 0);
    int findSequenceLen(String);
    bool drawSequence(String, uint16_t = 0);
    SdFat sd;
    Adafruit_HX8357 tft;
    Adafruit_ImageReader reader;

private:
    String frameName(String, int);
};

#endif