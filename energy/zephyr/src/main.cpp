#include <Adafruit_NeoPixel.h>
#include <Automaton.h>
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

/**
 * Machines representing the Tag in Range signals.
 */

Atm_digital tirDigitals[RFID_NUM];

const uint8_t TIR_PINS[RFID_NUM] = {
    A0, A1, A2
};

/**
 * LED strip.
 */

const uint16_t LED_NUM = 28;
const uint16_t LED_PIN = 8;
const uint8_t LED_BRIGHTNESS = 180;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(
    LED_NUM,
    LED_PIN,
    NEO_GRB + NEO_KHZ800);

Atm_timer timerLed;

const int LED_TIMER_MS = 250;

/**
 * Relays.
 */

const unsigned long RELAY_UNLOCK_MIN_MS = 3000;

const uint8_t RELAY_PINS[RFID_NUM] = {
    9, 10, 11
};

/**
 * Program state.
 */

char tagBuffer[SIZE_TAG_ID];

bool rfidsUnlocked[RFID_NUM];
bool tagsInRange[RFID_NUM];
unsigned long unlockMillis[RFID_NUM];

typedef struct programState {
    bool* rfidsUnlocked;
    bool* tagsInRange;
    unsigned long* unlockMillis;
} ProgramState;

ProgramState progState;

void initState()
{
    progState.rfidsUnlocked = rfidsUnlocked;
    progState.tagsInRange = tagsInRange;
    progState.unlockMillis = unlockMillis;

    for (int i = 0; i < RFID_NUM; i++) {
        progState.rfidsUnlocked[i] = false;
        progState.tagsInRange[i] = false;
        progState.unlockMillis[i] = 0;
    }
}

uint16_t getNumUnlocked()
{
    uint16_t ret = 0;

    for (int i = 0; i < RFID_NUM; i++) {
        if (progState.rfidsUnlocked[i]) {
            ret++;
        }
    }

    return ret;
}

/**
 * LED functions.
 */

uint32_t randomColor()
{
    return Adafruit_NeoPixel::Color(
        random(0, 255),
        random(0, 255),
        random(0, 255));
}

void onLedTimer(int idx, int v, int up)
{
    if (getNumUnlocked() >= RFID_NUM) {
        ledStrip.fill(randomColor());
        ledStrip.show();
    }
}

void initLeds()
{
    ledStrip.begin();
    ledStrip.setBrightness(LED_BRIGHTNESS);
    ledStrip.show();
    ledStrip.clear();

    timerLed
        .begin(LED_TIMER_MS)
        .repeat(-1)
        .onTimer(onLedTimer)
        .start();
}

void showStartEffect()
{
    const uint16_t delayMsFill = 500;
    const uint16_t delayMsClear = 100;
    const uint16_t numLoops = 3;
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

void showUnlockEffect(uint8_t idx)
{
    const uint32_t color = Adafruit_NeoPixel::Color(0, 250, 250);
    const unsigned long delayMs = 80;

    if (idx >= RFID_NUM) {
        Serial.print(F("WARN: Unlock effect for #"));
        Serial.println(idx);
        return;
    }

    uint16_t numPixels = ledStrip.numPixels();
    uint16_t pixelsPerRfid = ceil(numPixels / RFID_NUM);
    uint16_t idxIni = pixelsPerRfid * idx;
    uint16_t idxEnd = idxIni + pixelsPerRfid;
    idxEnd = idxEnd > numPixels ? numPixels : idxEnd;

    for (uint16_t i = idxIni; i < idxEnd; i++) {
        ledStrip.setPixelColor(i, color);
        ledStrip.show();
        delay(delayMs);
    }
}

void ledLoop()
{
    const unsigned long delayMs = 100;

    if (getNumUnlocked() >= RFID_NUM) {
        ledStrip.fill(randomColor());
        ledStrip.show();
        delay(delayMs);
    }
}

/**
 * Relay functions.
 */

void lockRelay(uint8_t idx)
{
    if (digitalRead(RELAY_PINS[idx]) == HIGH) {
        Serial.print(F("Locking relay #"));
        Serial.println(idx);
    }

    digitalWrite(RELAY_PINS[idx], LOW);
}

void openRelay(uint8_t idx)
{
    if (digitalRead(RELAY_PINS[idx]) == LOW) {
        Serial.print(F("Opening relay #"));
        Serial.println(idx);
    }

    digitalWrite(RELAY_PINS[idx], HIGH);
}

void initRelays()
{
    for (int i = 0; i < RFID_NUM; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        lockRelay(i);
    }
}

bool canLockRelay(uint8_t idx)
{
    unsigned long now = millis();

    if (progState.unlockMillis[idx] == 0
        || progState.unlockMillis[idx] > now) {
        return true;
    }

    unsigned long diff = now - progState.unlockMillis[idx];

    return diff >= RELAY_UNLOCK_MIN_MS;
}

void relayLoop()
{
    unsigned long now = millis();

    for (int i = 0; i < RFID_NUM; i++) {
        if (progState.tagsInRange[i]) {
            progState.unlockMillis[i] = now;
            openRelay(i);
        } else {
            if (canLockRelay(i)) {
                lockRelay(i);
            }
        }
    }
}

/**
 * RFID functions.
 */

void onTirChange(int idx, int v, int up)
{
    bool isInRange = v == 1;

    Serial.print(F("TIR #"));
    Serial.print(idx);
    Serial.print(F(": "));
    Serial.println(isInRange);

    progState.tagsInRange[idx] = isInRange;

    if (isInRange && progState.rfidsUnlocked[idx] == false) {
        Serial.print(F("Unlocked #"));
        Serial.println(idx);
        uint16_t numUnlocked = getNumUnlocked();
        progState.rfidsUnlocked[idx] = true;
        showUnlockEffect(numUnlocked);
    }
}

void initRfidsTagInRange()
{
    const uint16_t debounceMs = 50;
    const bool activeLow = false;
    const bool pullUp = false;

    for (int i = 0; i < RFID_NUM; i++) {
        tirDigitals[i]
            .begin(TIR_PINS[i], debounceMs, activeLow, pullUp)
            .onChange(onTirChange, i);
    }
}

void initRfids()
{
    for (int i = 0; i < RFID_NUM; i++) {
        rfidSoftSerials[i].begin(9600);
    }

    initRfidsTagInRange();
}

void rfidLoop()
{
    const uint8_t activeIdx = 0;

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

    Serial.print(F("Tag on #"));
    Serial.print(activeIdx);
    Serial.print(F(": "));
    Serial.println(tagBuffer);
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

    showStartEffect();

    Serial.println(F(">> Starting Zephyr"));
}

void loop()
{
    rfidLoop();
    relayLoop();
    automaton.run();
}