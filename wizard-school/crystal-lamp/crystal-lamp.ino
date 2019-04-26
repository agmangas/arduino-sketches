#include <Adafruit_NeoPixel.h>
#include "rdm630.h"

/**
 * Program states.
 */

const byte NUM_STAGES = 3;

typedef struct programState
{
    bool isStageCompleted[NUM_STAGES];
    bool isRelayOpen[NUM_STAGES];
} ProgramState;

ProgramState progState = {
    .isStageCompleted = {false, false, false},
    .isRelayOpen = {false, false, false}};

/**
 * Pins.
 */

const byte PIN_LEDS = 3;

const byte PIN_AUDIO_T0 = 4;
const byte PIN_AUDIO_T1 = 5;
const byte PIN_AUDIO_RST = 6;
const byte PIN_AUDIO_ACT = 7;

const byte RELAY_PINS[NUM_STAGES] = {
    8, 9, 10};

// Arduino RX pin <--> RFID reader TX pin

const byte PIN_RFID_01_RX = 2;
const byte PIN_RFID_01_TX = A0;
const byte PIN_RFID_02_RX = 11;
const byte PIN_RFID_02_TX = A1;
const byte PIN_RFID_03_RX = 12;
const byte PIN_RFID_03_TX = A2;

/**
 * LED strip.
 */

const byte DEFAULT_BRIGHTNESS = 150;

const uint32_t LED_COLORS[NUM_STAGES] = {
    Adafruit_NeoPixel::Color(255, 0, 0),
    Adafruit_NeoPixel::Color(0, 255, 0),
    Adafruit_NeoPixel::Color(0, 0, 255)};

// sum(LED_STAGE_PATCH_SIZES) == NUM_LEDS

const byte NUM_LEDS = 60;

const byte LED_STAGE_PATCH_SIZES[NUM_STAGES] = {
    20, 20, 20};

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

/**
 * RFID readers.
 */

RDM6300 rfid01(PIN_RFID_01_RX, PIN_RFID_01_TX);
RDM6300 rfid02(PIN_RFID_02_RX, PIN_RFID_02_TX);
RDM6300 rfid03(PIN_RFID_03_RX, PIN_RFID_03_TX);

RDM6300 rfidReaders[NUM_STAGES] = {
    rfid01,
    rfid02,
    rfid03};

String validStageTags[NUM_STAGES] = {
    "1D00277FBDF8",
    "1D00278D53E4",
    "1D0027B80A88"};

const byte NUM_RESET_TAGS = 1;

String resetTags[NUM_RESET_TAGS] = {
    "112233445566"};

/**
 * LED functions.
 */

void initLedStrip()
{
    ledStrip.begin();
    ledStrip.setBrightness(DEFAULT_BRIGHTNESS);
    ledStrip.clear();
    ledStrip.show();
}

void showStageLeds(int idx)
{
    if (idx >= NUM_STAGES)
    {
        return;
    }

    int iniIdx = 0;

    for (int i = 0; i < idx; i++)
    {
        iniIdx += LED_STAGE_PATCH_SIZES[i];
    }

    int endIdx = iniIdx + LED_STAGE_PATCH_SIZES[idx];

    uint32_t color = LED_COLORS[idx];

    for (int i = iniIdx; i < endIdx; i++)
    {
        ledStrip.setPixelColor(i, color);
    }

    ledStrip.show();
}

/**
 * Audio FX board functions.
 */

void playTrack(byte trackPin)
{
    if (isTrackPlaying())
    {
        Serial.println(F("Skipping: Audio still playing"));
        return;
    }

    Serial.print(F("Playing track on pin: "));
    Serial.println(trackPin);

    digitalWrite(trackPin, LOW);
    pinMode(trackPin, OUTPUT);
    delay(300);
    pinMode(trackPin, INPUT);
}

void resetAudio()
{
    Serial.println(F("Audio FX reset"));

    digitalWrite(PIN_AUDIO_RST, LOW);
    pinMode(PIN_AUDIO_RST, OUTPUT);
    delay(10);
    pinMode(PIN_AUDIO_RST, INPUT);

    Serial.println(F("Waiting for Audio FX startup"));

    delay(2000);
}

void initAudioPins()
{
    pinMode(PIN_AUDIO_T0, INPUT);
    pinMode(PIN_AUDIO_T1, INPUT);
    pinMode(PIN_AUDIO_ACT, INPUT);
    pinMode(PIN_AUDIO_RST, INPUT);
}

bool isTrackPlaying()
{
    return digitalRead(PIN_AUDIO_ACT) == LOW;
}

/**
 * Relay functions.
 */

void lockRelay(byte idx)
{
    digitalWrite(RELAY_PINS[idx], LOW);
    progState.isRelayOpen[idx] = false;
}

void openRelay(byte idx)
{
    digitalWrite(RELAY_PINS[idx], HIGH);
    progState.isRelayOpen[idx] = true;
}

void initRelay(byte idx)
{
    pinMode(RELAY_PINS[idx], OUTPUT);
    lockRelay(idx);
}

void initRelays()
{
    for (int i = 0; i < NUM_STAGES; i++)
    {
        initRelay(i);
    }
}

void updateRelay(byte idx)
{
    if (progState.isStageCompleted[idx] &&
        !progState.isRelayOpen[idx])
    {
        openRelay(idx);
    }
    else if (!progState.isStageCompleted[idx] &&
             progState.isRelayOpen[idx])
    {
        lockRelay(idx);
    }
}

void updateRelays()
{
    for (int i = 0; i < NUM_STAGES; i++)
    {
        updateRelay(i);
    }
}

/**
 * RFID functions.
 */

void onValidStage(int idx)
{
    const unsigned long OPEN_RELAY_SLEEP_MS = 1000;
    const unsigned long AUDIO_WAIT_SLEEP_MS = 10;
    const unsigned long MAX_AUDIO_WAIT_MS = 10000;

    unsigned long ini;
    unsigned long now;

    progState.isStageCompleted[idx] = true;

    Serial.println(F("Showing LEDs & playing audio"));

    showStageLeds(idx);
    playTrack(PIN_AUDIO_T0);

    Serial.println(F("Waiting for audio"));

    ini = millis();

    while (isTrackPlaying())
    {
        delay(AUDIO_WAIT_SLEEP_MS);

        now = millis();

        if ((now < ini) || ((now - ini) > MAX_AUDIO_WAIT_MS))
        {
            Serial.println(F("Max audio wait: Breaking"));
            break;
        }
    }

    Serial.print(F("Sleeping for (ms): "));
    Serial.println(OPEN_RELAY_SLEEP_MS);

    delay(OPEN_RELAY_SLEEP_MS);

    Serial.println(F("Opening relay"));

    updateRelay(idx);
}

void pollRfidReaders()
{
    String tagId;

    for (int i = 0; i < NUM_STAGES; i++)
    {
        tagId = rfidReaders[i].getTagId();

        if (!tagId.length())
        {
            continue;
        }

        Serial.print(F("Tag #"));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.println(tagId);

        if (validStageTags[i].compareTo(tagId) == 0 &&
            !progState.isStageCompleted[i])
        {
            Serial.print(F("Valid tag on #"));
            Serial.println(i);

            onValidStage(i);
        }

        for (int j = 0; j < NUM_RESET_TAGS; j++)
        {
            if (resetTags[j].compareTo(tagId) == 0)
            {
                Serial.println(F("Reset tag detected"));
                resetGame();
                break;
            }
        }
    }
}

/**
 * Utility functions.
 */

void resetGame()
{
    ledStrip.clear();
    ledStrip.show();

    for (int i = 0; i < NUM_STAGES; i++)
    {
        lockRelay(i);
        progState.isStageCompleted[i] = false;
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initAudioPins();
    initRelays();
    initLedStrip();
    resetAudio();

    Serial.println(">> Starting crystal lamp program");
    Serial.flush();
}

void loop()
{
    pollRfidReaders();
    updateRelays();
}
