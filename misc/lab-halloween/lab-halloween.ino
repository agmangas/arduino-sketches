#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>
#include "rdm630.h"

/**
 * RFID reader.
 */

const byte PIN_RFID_RX = 11;
const byte PIN_RFID_TX = 13;

RDM6300 rfidReader(PIN_RFID_RX, PIN_RFID_TX);

const byte NUM_VALID_TAGS = 2;
const byte NUM_RESET_TAGS = 1;

String validTags[NUM_VALID_TAGS] = {
    "1D0027793200",
    "1D0027AEA800"};

String resetTags[NUM_RESET_TAGS] = {
    "1D0027793200"};

const byte RFID_TAG_NONE = 0;
const byte RFID_TAG_VALID = 1;
const byte RFID_TAG_RESET = 2;

/**
 * Throughhole LEDs.
 */

const int LED_THROUGH_BRIGHTNESS = 250;
const int LED_THROUGH_NUM = 3;
const int LED_THROUGH_PIN = 12;

Adafruit_NeoPixel ledThrough = Adafruit_NeoPixel(
    LED_THROUGH_NUM,
    LED_THROUGH_PIN,
    NEO_RGB + NEO_KHZ800);

const uint32_t LED_THROUGH_COLOR = Adafruit_NeoPixel::Color(255, 0, 255);

const int LED_THROUGH_IDX_BUTTON = 0;
const int LED_THROUGH_IDX_BUTTON_WLED = 1;
const int LED_THROUGH_IDX_RFID = 2;

/**
 * Buttons (w/ LED).
 */

const byte LED_STATE_OFF = 0;
const byte LED_STATE_ON = 1;
const byte LED_STATE_BLINK = 2;

const int BUTTON_WLED_NUM = 5;

const int BUTTON_WLED_PINS[BUTTON_WLED_NUM] = {
    2, 3, 4, 5, 6};

const int LED_PINS[BUTTON_WLED_NUM] = {
    7, 8, 9, 10, A0};

const int LED_DURATION_MS = 50;
const int LED_DURATION_PAUSE_MS = 350;

Atm_button buttonsLed[BUTTON_WLED_NUM];
Atm_led leds[BUTTON_WLED_NUM];

const byte BUTTON_WLED_KEY[BUTTON_WLED_NUM] = {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINK,
    LED_STATE_OFF,
    LED_STATE_ON};

/**
 * Buttons (w/o LED).
 */

const int BUTTON_NUM = 4;
const int BUTTON_BUF_SIZE = 30;

const int BUTTON_PINS[BUTTON_NUM] = {
    A1, A2, A3, A4};

Atm_button buttons[BUTTON_NUM];

CircularBuffer<byte, BUTTON_BUF_SIZE> buttonsBuf;

const int BUTTON_KEY_SIZE = 8;

const int BUTTON_KEY[BUTTON_KEY_SIZE] = {
    0, 0, 1, 1, 2, 2, 3, 3};

/**
 * Relay.
 */

const int RELAY_PIN = A5;

/**
 * Program state.
 */

byte buttonsLedState[BUTTON_WLED_NUM];

typedef struct programState
{
    byte *buttonsLedState;
    bool isButtonsLedOk;
    bool isButtonsOk;
    bool isRfidReaderOk;
    bool isComplete;
} ProgramState;

ProgramState progState = {
    .buttonsLedState = buttonsLedState,
    .isButtonsLedOk = false,
    .isButtonsOk = false,
    .isRfidReaderOk = false,
    .isComplete = false};

void initState()
{
    for (int i = 0; i < BUTTON_WLED_NUM; i++)
    {
        progState.buttonsLedState[i] = LED_STATE_OFF;
    }

    progState.isButtonsLedOk = false;
    progState.isButtonsOk = false;
    progState.isRfidReaderOk = false;
    progState.isComplete = false;

    buttonsBuf.clear();
}

void resetState()
{
    initState();

    for (int i = 0; i < BUTTON_WLED_NUM; i++)
    {
        leds[i].trigger(Atm_led::EVT_OFF);
    }

    lockRelay();
}

bool shouldPollRfidReader()
{
    return progState.isButtonsOk == true &&
           progState.isButtonsLedOk == true;
}

bool shouldRunCompletedCallback()
{
    return progState.isButtonsOk == true &&
           progState.isButtonsLedOk == true &&
           progState.isRfidReaderOk == true &&
           progState.isComplete == false;
}

void refreshState()
{
    if (progState.isButtonsOk == false &&
        isButtonsStateValid())
    {
        Serial.println(F("Buttons: OK"));
        progState.isButtonsOk = true;
        setLedThroughButtons();
    }

    if (progState.isButtonsLedOk == false &&
        isButtonsLedStateValid())
    {
        Serial.println(F("Buttons w/LED: OK"));
        progState.isButtonsLedOk = true;
        setLedThroughButtonsLed();
    }

    if (shouldPollRfidReader())
    {
        byte currTag = pollRfidReader();

        if (currTag == RFID_TAG_VALID)
        {
            Serial.println(F("RFID: OK"));
            progState.isRfidReaderOk = true;
            setLedThroughRfid();
        }
        else if (currTag == RFID_TAG_RESET)
        {
            Serial.println(F("RFID: Reset"));
            resetState();
        }
    }

    if (shouldRunCompletedCallback())
    {
        onCompleted();
    }
}

void onCompleted()
{
    Serial.println(F("Completed"));
    progState.isComplete = true;
    flashLedThrough();
    openRelay();
}

/**
 * Throughhole LED functions.
 */

void flashLedThrough()
{
    const int iterNum = 30;
    const int delayMs = 100;

    for (int i = 0; i < iterNum; i++)
    {
        ledThrough.clear();
        ledThrough.show();
        delay(delayMs);
        ledThrough.fill(LED_THROUGH_COLOR);
        ledThrough.show();
        delay(delayMs);
    }

    ledThrough.clear();
    ledThrough.show();
}

void showLedThrough(int idx)
{
    ledThrough.setPixelColor(idx, LED_THROUGH_COLOR);
    ledThrough.show();
}

void setLedThroughButtons()
{
    showLedThrough(LED_THROUGH_IDX_BUTTON);
}

void setLedThroughButtonsLed()
{
    showLedThrough(LED_THROUGH_IDX_BUTTON_WLED);
}

void setLedThroughRfid()
{
    showLedThrough(LED_THROUGH_IDX_RFID);
}

void initLedThrough()
{
    ledThrough.begin();
    ledThrough.setBrightness(LED_THROUGH_BRIGHTNESS);
    ledThrough.clear();
    ledThrough.show();
}

/**
 * Buttons (w/ LED) functions.
 */

bool isButtonsLedStateValid()
{
    for (int i = 0; i < BUTTON_WLED_NUM; i++)
    {
        if (progState.buttonsLedState[i] != BUTTON_WLED_KEY[i])
        {
            return false;
        }
    }

    return true;
}

void onPressButtonsLed(int idx, int v, int up)
{
    Serial.print(F("Button w/LED: "));
    Serial.println(idx);

    if (progState.buttonsLedState[idx] == LED_STATE_OFF)
    {
        Serial.println(F("ON"));
        progState.buttonsLedState[idx] = LED_STATE_ON;
        leds[idx].trigger(Atm_led::EVT_ON);
    }
    else if (progState.buttonsLedState[idx] == LED_STATE_ON)
    {
        Serial.println(F("Blink"));
        progState.buttonsLedState[idx] = LED_STATE_BLINK;
        leds[idx].trigger(Atm_led::EVT_BLINK);
    }
    else
    {
        Serial.println(F("OFF"));
        progState.buttonsLedState[idx] = LED_STATE_OFF;
        leds[idx].trigger(Atm_led::EVT_OFF);
    }
}

void initButtonsLed()
{
    for (int i = 0; i < BUTTON_WLED_NUM; i++)
    {
        buttonsLed[i]
            .begin(BUTTON_WLED_PINS[i])
            .onPress(onPressButtonsLed, i);

        leds[i]
            .begin(LED_PINS[i])
            .blink(LED_DURATION_MS, LED_DURATION_PAUSE_MS);

        leds[i]
            .trigger(Atm_led::EVT_OFF);
    }
}

/**
 * Buttons (w/o LED) functions.
 */

bool isButtonsStateValid()
{
    if (buttonsBuf.size() < BUTTON_KEY_SIZE)
    {
        return false;
    }

    int buttonsBufLimitHi = buttonsBuf.size() - BUTTON_KEY_SIZE;
    bool isValid;

    for (int i = 0; i < buttonsBufLimitHi; i++)
    {
        isValid = true;

        for (int j = 0; j < BUTTON_KEY_SIZE; j++)
        {
            if (buttonsBuf[i + j] != BUTTON_KEY[j])
            {
                isValid = false;
                break;
            }
        }

        if (isValid)
        {
            return true;
        }
    }

    return false;
}

void onPressButtons(int idx, int v, int up)
{
    Serial.print(F("Button: "));
    Serial.println(idx);

    buttonsBuf.push(idx);
}

void initButtons()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        buttons[i]
            .begin(BUTTON_PINS[i])
            .onPress(onPressButtons, i);
    }
}

/**
 * RFID functions.
 */

void initRfid()
{
    rfidReader.begin();
}

byte pollRfidReader()
{
    String tagId;

    Serial.println(F("Reading RFID"));

    tagId = rfidReader.getTagId();

    if (!tagId.length())
    {
        return;
    }

    Serial.print(F("Tag :: "));
    Serial.println(tagId);

    for (int i = 0; i < NUM_VALID_TAGS; i++)
    {
        if (validTags[i].compareTo(tagId) == 0)
        {
            return RFID_TAG_VALID;
        }
    }

    for (int i = 0; i < NUM_RESET_TAGS; i++)
    {
        if (resetTags[i].compareTo(tagId) == 0)
        {
            return RFID_TAG_RESET;
        }
    }

    return RFID_TAG_NONE;
}

/**
 * Relay functions.
 */

void lockRelay()
{
    digitalWrite(RELAY_PIN, LOW);
}

void openRelay()
{
    digitalWrite(RELAY_PIN, HIGH);
}

void initRelay()
{
    pinMode(RELAY_PIN, OUTPUT);
    lockRelay();
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initButtonsLed();
    initButtons();
    initLedThrough();
    initRelay();
    initRfid();

    Serial.println(F(">> Starting Halloween lab program"));
}

void loop()
{
    automaton.run();
    refreshState();
}