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

const uint16_t NUM_LEDS = 10;
const uint16_t PIN_LEDS = 4;

const int LED_EFFECT_STEP_MS = 3;
const int LED_BRIGHTNESS = 180;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(
    NUM_LEDS,
    PIN_LEDS,
    NEO_GRB + NEO_KHZ800);

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
    pixelStrip.clear();
    pixelStrip.show();
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
            pixelStrip.setPixelColor(i, trackColor);
            pixelStrip.show();
            delay(LED_EFFECT_STEP_MS);
        }

        for (int i = (currTarget - 1); i >= 0; i--) {
            pixelStrip.setPixelColor(i, 0);
            pixelStrip.show();
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
    pixelStrip.begin();
    pixelStrip.setBrightness(LED_BRIGHTNESS);
    pixelStrip.show();

    clearLeds();
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
    sSerial.listen();

    Serial.println(F(">> Starting Cerebrofono program"));
}

void loop()
{
    readTagAndPlayAudio();
    delay(100);
}
