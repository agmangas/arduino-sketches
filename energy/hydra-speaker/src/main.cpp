#include <Adafruit_NeoPixel.h>
#include <SerialRFID.h>
#include <SoftwareSerial.h>

/**
 * RFID reader.
 */

const byte PIN_RFID_RX = A0;
const byte PIN_RFID_TX = A1;

SoftwareSerial sSerial(PIN_RFID_RX, PIN_RFID_TX);
SerialRFID rfid(sSerial);

char tag[SIZE_TAG_ID];

/**
 * Valid tag IDs.
 */

const int NUM_TRACKS = 4;
const int NUM_IDS_PER_TRACK = 2;

char TRACK_TAG_IDS[NUM_TRACKS][NUM_IDS_PER_TRACK][SIZE_TAG_ID] = {
    { "1D0027A729B4", "1D0027A729B4" },
    { "1D00279848EA", "1D00279848EA" },
    { "1D0027E11DC6", "1D0027E11DC6" },
    { "1D0027A2AE36", "1D0027A2AE36" }
};

/**
 * Audio FX.
 */

const byte PIN_AUDIO_RST = 12;
const byte PIN_AUDIO_ACT = 7;

const byte AUDIO_TRACK_PINS[NUM_TRACKS] = {
    8, 9, 10, 11
};

const unsigned long AUDIO_TRACK_MAX_MS = 50000;
const int AUDIO_EFFECT_DELAY_MS = 500;

/**
 * LED strip.
 */

const uint16_t NUM_LEDS = 60;
const uint16_t NUM_LEDS_STATIC = 25;

const uint8_t PIN_LEDS_01 = 3;
const uint8_t PIN_LEDS_02 = 4;
const uint8_t PIN_LEDS_STATIC = A5;

const int LED_EFFECT_STEP_MS = 3;
const int LED_BRIGHTNESS = 250;

Adafruit_NeoPixel pixelStrip01 = Adafruit_NeoPixel(
    NUM_LEDS,
    PIN_LEDS_01,
    NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel pixelStrip02 = Adafruit_NeoPixel(
    NUM_LEDS,
    PIN_LEDS_02,
    NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel pixelStatic = Adafruit_NeoPixel(
    NUM_LEDS_STATIC,
    PIN_LEDS_STATIC,
    NEO_GRB + NEO_KHZ800);

const uint32_t COLOR_STATIC = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(200, 200, 200));

const uint32_t AUDIO_TRACK_COLORS[NUM_TRACKS] = {
    Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(0, 50, 255)),
    Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(0, 100, 255)),
    Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(0, 150, 255)),
    Adafruit_NeoPixel::gamma32(
        Adafruit_NeoPixel::Color(0, 200, 255))
};

/**
 * Audio FX functions.
 */

bool isTrackPlaying()
{
    return digitalRead(PIN_AUDIO_ACT) == LOW;
}

void playTrack(byte trackPin)
{
    if (isTrackPlaying()) {
        Serial.println(F("Skipping: Audio still playing"));
        return;
    }

    Serial.print(F("Playing track on pin: "));
    Serial.println(trackPin);

    digitalWrite(trackPin, LOW);
    pinMode(trackPin, OUTPUT);
    delay(200);
    pinMode(trackPin, INPUT);
}

void initAudioPins()
{
    for (int i = 0; i < NUM_TRACKS; i++) {
        pinMode(AUDIO_TRACK_PINS[i], INPUT);
    }

    pinMode(PIN_AUDIO_ACT, INPUT);
    pinMode(PIN_AUDIO_RST, INPUT);
}

void resetAudio()
{
    Serial.println(F("Audio FX reset"));

    digitalWrite(PIN_AUDIO_RST, LOW);
    pinMode(PIN_AUDIO_RST, OUTPUT);
    delay(100);
    pinMode(PIN_AUDIO_RST, INPUT);

    Serial.println(F("Waiting for Audio FX startup"));

    delay(2000);
}

/**
 * LED strip functions.
 */

void clearLeds()
{
    pixelStrip01.clear();
    pixelStrip01.show();

    pixelStrip02.clear();
    pixelStrip02.show();
}

void displayAudioLedEffect(int tagIdx)
{
    int limitLo = floor(NUM_LEDS * 0.45);
    int limitHi = floor(NUM_LEDS * 1.00);

    uint32_t trackColor = AUDIO_TRACK_COLORS[tagIdx];

    unsigned long now;
    unsigned long ini = millis();
    bool timeout = false;

    int currTarget;

    delay(AUDIO_EFFECT_DELAY_MS);

    while (isTrackPlaying() && !timeout) {
        currTarget = random(limitLo, limitHi);

        clearLeds();

        for (int i = 0; i < currTarget; i++) {
            pixelStrip01.setPixelColor(i, trackColor);
            pixelStrip01.show();
            pixelStrip02.setPixelColor(i, trackColor);
            pixelStrip02.show();
            delay(LED_EFFECT_STEP_MS);
        }

        for (int i = (currTarget - 1); i >= 0; i--) {
            pixelStrip01.setPixelColor(i, 0);
            pixelStrip01.show();
            pixelStrip02.setPixelColor(i, 0);
            pixelStrip02.show();
            delay(LED_EFFECT_STEP_MS);
        }

        now = millis();

        if (now < ini) {
            Serial.println(F("Audio timeout: clock overflow"));
            timeout = true;
        } else if ((now - ini) > AUDIO_TRACK_MAX_MS) {
            Serial.println(F("Audio timeout"));
            timeout = true;
        }
    }

    clearLeds();
}

void initLeds()
{
    pixelStrip01.begin();
    pixelStrip01.setBrightness(LED_BRIGHTNESS);
    pixelStrip01.show();

    pixelStrip02.begin();
    pixelStrip02.setBrightness(LED_BRIGHTNESS);
    pixelStrip02.show();

    pixelStatic.begin();
    pixelStatic.setBrightness(LED_BRIGHTNESS);
    pixelStatic.show();

    clearLeds();
}

void playLedStartupPattern()
{
    clearLeds();

    const int delayMs = 5;
    const uint32_t color = Adafruit_NeoPixel::Color(200, 200, 200);

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        pixelStrip01.setPixelColor(i, color);
        pixelStrip01.show();
        delay(delayMs);
    }

    clearLeds();
}

void showStaticLed()
{
    pixelStatic.clear();

    for (uint16_t i = 0; i < NUM_LEDS_STATIC; i++) {
        pixelStatic.setPixelColor(i, COLOR_STATIC);
    }

    pixelStatic.show();
}

/**
 * RFID reader functions.
 */

int readCurrentTagIndex()
{
    bool tagFound = rfid.readTag(tag, sizeof(tag));

    if (!tagFound) {
        return -1;
    }

    Serial.print(F("Tag: "));
    Serial.println(tag);

    for (int i = 0; i < NUM_TRACKS; i++) {
        for (int j = 0; j < NUM_IDS_PER_TRACK; j++) {
            if (SerialRFID::isEqualTag(tag, TRACK_TAG_IDS[i][j])) {
                Serial.print(F("Track match: "));
                Serial.println(i);
                return i;
            }
        }
    }

    return -1;
}

void readTagAndPlayAudio()
{
    int tagIdx = readCurrentTagIndex();

    if (tagIdx == -1) {
        return;
    }

    playTrack(AUDIO_TRACK_PINS[tagIdx]);
    displayAudioLedEffect(tagIdx);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);
    sSerial.begin(9600);

    initAudioPins();
    resetAudio();
    initLeds();
    playLedStartupPattern();
    sSerial.listen();

    Serial.println(F(">> Starting Cerebrofono program"));
}

void loop()
{
    readTagAndPlayAudio();
    showStaticLed();
    delay(100);
}
