#include <Adafruit_NeoPixel.h>
#include <SerialRFID.h>
#include <SoftwareSerial.h>

/**
 * RFID readers.
 */

const uint8_t RFID_NUM = 3;

const uint8_t RFID_PINS_RX[RFID_NUM] = {
    2, 4, 6
};

const uint8_t RFID_PINS_TX[RFID_NUM] = {
    3, 5, 7
};

SoftwareSerial rfidSoftSerials[RFID_NUM] = {
    SoftwareSerial(RFID_PINS_RX[0], RFID_PINS_TX[0]),
    SoftwareSerial(RFID_PINS_RX[1], RFID_PINS_TX[1]),
    SoftwareSerial(RFID_PINS_RX[2], RFID_PINS_TX[2])
};

SerialRFID rfids[RFID_NUM] = {
    SerialRFID(rfidSoftSerials[0]),
    SerialRFID(rfidSoftSerials[1]),
    SerialRFID(rfidSoftSerials[2])
};

const uint8_t RFID_VALID_OPTIONS = 2;

char keyTags[RFID_NUM][RFID_VALID_OPTIONS][SIZE_TAG_ID] = {
    { "5C00CADB5A19", "5C00CADB5A19" },
    { "5C00CADB5A19", "5C00CADB5A19" },
    { "5C00CADB5A19", "5C00CADB5A19" }
};

/**
 * LED strip.
 */

const uint16_t LED_NUM = 60;
const uint16_t LED_PIN = 8;
const uint8_t LED_BRIGHTNESS = 200;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(
    LED_NUM,
    LED_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * Relays.
 */

const uint8_t RELAY_PINS[RFID_NUM] = {
    9, 10, 11
};

/**
 * Program state.
 */

char tagBuffer[SIZE_TAG_ID];
bool rfidsUnlocked[RFID_NUM];

typedef struct programState {
    bool* rfidsUnlocked;
} ProgramState;

ProgramState progState;

void initState()
{
    for (int i = 0; i < RFID_NUM; i++) {
        progState.rfidsUnlocked[i] = false;
    }
}

/**
 * Relay functions.
 */

void lockRelay(uint8_t idx)
{
    digitalWrite(RELAY_PINS[idx], LOW);
}

void openRelay(uint8_t idx)
{
    digitalWrite(RELAY_PINS[idx], HIGH);
}

void initRelays()
{
    for (uint8_t i = 0; i < RFID_NUM; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        lockRelay(RELAY_PINS[i]);
    }
}

/**
 * RFID functions.
 */

void initRfids()
{
    for (int i = 0; i < RFID_NUM; i++) {
        rfidSoftSerials[i].begin(9600);
    }

    rfidSoftSerials[0].listen();
    Serial.print(F("Listening on #0"));
}

uint8_t activeRfidIndex()
{
    for (int i = 0; i < RFID_NUM; i++) {
        if (progState.rfidsUnlocked[i] == false) {
            return i;
        }
    }

    return 0;
}

void onValidTag(uint8_t readerIdx)
{
    if (progState.rfidsUnlocked[readerIdx]) {
        return;
    }

    Serial.print(F("Valid tag on #"));
    Serial.println(readerIdx);

    progState.rfidsUnlocked[readerIdx] = true;
    openRelay(RELAY_PINS[readerIdx]);
}

void rfidLoop()
{
    uint8_t activeIdx = activeRfidIndex();

    bool isReplaced = rfidSoftSerials[activeIdx].listen();

    if (isReplaced) {
        Serial.print(F("Listening on #"));
        Serial.println(activeIdx);
    }

    bool someTag = rfids[activeIdx].readTag(
        tagBuffer,
        sizeof(tagBuffer));

    if (!someTag) {
        return;
    }

    Serial.print("Tag: ");
    Serial.print(tagBuffer);

    bool isValidTag = false;

    for (int opt = 0; opt < RFID_VALID_OPTIONS; opt++) {
        isValidTag = SerialRFID::isEqualTag(
            tagBuffer,
            keyTags[activeIdx][opt]);

        if (isValidTag) {
            break;
        }
    }

    if (isValidTag) {
        onValidTag(activeIdx);
    }
}

/**
 * LED functions.
 */

void initLeds()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_BRIGHTNESS);
    ledStrip.show();
    ledStrip.clear();
}

void showStartEffect()
{
    const uint16_t delayMsFill = 1000;
    const uint16_t delayMsClear = 200;
    const uint16_t numLoops = 2;
    const uint32_t color = Adafruit_NeoPixel::Color(0, 255, 0);

    for (uint16_t i = 0; i < numLoops; i++) {
        ledStrip.fill(color);
        ledStrip.show();
        delay(delayMsFill);
        ledStrip.clear();
        ledStrip.show();
        delay(delayMsClear);
    }

    ledStrip.clear();
    ledStrip.show();
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initRfids();
    initLeds();
    initRelays();

    Serial.println(F(">> Starting zephyr"));

    showStartEffect();
}

void loop()
{
    rfidLoop();
}