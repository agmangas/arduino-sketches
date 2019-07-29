#include <Adafruit_NeoPixel.h>
#include <Automaton.h>

/**
 * Potentiometers.
 */

const int POTS_NUM = 4;

const int POTS_PINS[POTS_NUM] = {
    A7, A6, A5, A4};

Atm_analog pots[POTS_NUM];
Atm_controller potsControl;

const byte POTS_RANGE_LO = 0;
const byte POTS_RANGE_HI = 20;

const int POTS_KEY[POTS_NUM] = {
    2, 2, 2, 2};

const int POTS_BOUNCE_MS = 1000;

/**
 * Relays.
 * Pots relay: Open on valid potentiometers combination.
 * Digital switches relay: Open on valid digital switches combination.
 * Microphone relay: Open on microphone max level.
 */

const int PIN_RELAY_POTS = 8;
const int PIN_RELAY_DSWITCHES = 7;
const int PIN_RELAY_TAP = 9;

/**
 * Digital switch locks.
 */

const int DSWITCH_NUM = 3;
const int DSWITCH_DEBOUNCE_MS = 1000;
const int DSWITCH_PINS[DSWITCH_NUM] = {5, 12, A0};
const int DSWITCH_LED_PINS[DSWITCH_NUM] = {2, 3, 4};

Atm_button dswitchButtons[DSWITCH_NUM];
Atm_led dswitchLeds[DSWITCH_NUM];
Atm_controller dswitchControl;

/**
 * Audio FX.
 */

const int PIN_AUDIO_TRACK_FILL = A3;
const int PIN_AUDIO_TRACK_TAP = A2;
const int PIN_AUDIO_ACT = 11;
const int PIN_AUDIO_RST = 10;

/**
 * Microphone.
 */

const int MICRO_PIN = A1;
const int MICRO_SAMPLERATE = 40;
const int MICRO_MAX_LEVEL = 15;
const int MICRO_TIMER_MS = 5000;
const int MICRO_THRESHOLD = 90;

Atm_analog microAnalog;
Atm_timer microTimer;

/**
 * LED strip.
 * Segment limits are (inclusive, exclusive).
 * Block 1: Potentiometers lock.
 * Block 2: From potentiometers lock to digital switches.
 * Block 3: From digital switches to microphone.
 * Block 4: Microphone progress indicator.
 */

const uint16_t LEDS_NUM = 200;
const uint8_t LEDS_PIN = 6;
const int LEDS_BRIGHTNESS = 150;
const int LEDS_FILL_MS = 80;
const int LEDS_BLINK_MS = 150;

// Lower value is always the lower limit (i.e. the inclusive index).

const int LEDS_POTS_SEGMENTS[POTS_NUM][2] = {
    {0, 20},
    {40, 20},
    {40, 60},
    {80, 60}};

const int LEDS_BLOCK2_SEGMENT[2] = {80, 110};
const int LEDS_BLOCK3_SEGMENT[2] = {158, 188};
const int LEDS_BLOCK4_SEGMENT[2] = {110, 158};

const uint32_t LEDS_COLOR_GREEN = Adafruit_NeoPixel::Color(57, 255, 20);
const uint32_t LEDS_COLOR_ORANGE = Adafruit_NeoPixel::Color(255, 70, 0);

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(LEDS_NUM, LEDS_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

int currPotValues[POTS_NUM];

typedef struct programState
{
    bool potsUnlocked;
    bool dswitchesUnlocked;
    bool tapUnlocked;
    int *currPotValues;
    unsigned long millisValidPots;
    int microLevel;
} ProgramState;

ProgramState progState = {
    .potsUnlocked = false,
    .dswitchesUnlocked = false,
    .tapUnlocked = false,
    .currPotValues = currPotValues,
    .millisValidPots = 0,
    .microLevel = 0};

/**
 * Potentiometer functions.
 */

bool isValidPotsCombination()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        if (progState.currPotValues[i] != POTS_KEY[i] &&
            progState.currPotValues[i] != (POTS_KEY[i] + 1) &&
            progState.currPotValues[i] != (POTS_KEY[i] - 1))
        {
            return false;
        }
    }

    return true;
}

bool isPotsUnlocked()
{
    if (progState.potsUnlocked)
    {
        return true;
    }

    if (!isValidPotsCombination())
    {
        progState.millisValidPots = 0;
        return false;
    }

    unsigned long now = millis();

    if (progState.millisValidPots == 0 ||
        progState.millisValidPots > now)
    {
        progState.millisValidPots = now;
        return false;
    }

    return (now - progState.millisValidPots) > POTS_BOUNCE_MS;
}

void onUnlockedPots()
{
    Serial.println(F("Unlocked pots"));
    progState.potsUnlocked = true;
    fillLedSegment(LEDS_BLOCK2_SEGMENT[0], LEDS_BLOCK2_SEGMENT[1]);
    openRelay(PIN_RELAY_POTS);
    refreshDswitches();
}

void onPotChange(int idx, int v, int up)
{
    if (progState.potsUnlocked)
    {
        return;
    }

    Serial.print(F("Pot["));
    Serial.print(idx);
    Serial.print(F("]:"));
    Serial.println(v);

    progState.currPotValues[idx] = v;

    refreshLedSegmentPots();
}

void initPots()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        progState.currPotValues[i] = POTS_RANGE_LO;
    }

    for (int i = 0; i < POTS_NUM; i++)
    {
        pots[i]
            .begin(POTS_PINS[i])
            .range(POTS_RANGE_LO, POTS_RANGE_HI)
            .onChange(onPotChange, i);
    }

    potsControl
        .begin()
        .IF(isPotsUnlocked)
        .onChange(true, onUnlockedPots);
}

/**
 * Digital switch functions.
 */

void refreshDswitches()
{
    if (!progState.potsUnlocked || progState.dswitchesUnlocked)
    {
        return;
    }

    for (int i = 0; i < DSWITCH_NUM; i++)
    {
        Serial.print(F("Dswitch:"));
        Serial.print(i);
        Serial.print(F(":"));

        if (dswitchButtons[i].state() == Atm_button::PRESSED)
        {
            Serial.println(F("ON"));
            dswitchLeds[i].trigger(Atm_led::EVT_ON);
        }
        else
        {
            Serial.println(F("OFF"));
            dswitchLeds[i].trigger(Atm_led::EVT_OFF);
        }
    }
}

bool allDswitchesPressed()
{
    if (progState.dswitchesUnlocked)
    {
        return true;
    }

    if (!progState.potsUnlocked)
    {
        return false;
    }

    for (int i = 0; i < DSWITCH_NUM; i++)
    {
        if (dswitchButtons[i].state() != Atm_button::PRESSED)
        {
            return false;
        }
    }

    return true;
}

void onUnlockedDswitches()
{
    Serial.println(F("Unlocked dswitches"));
    progState.dswitchesUnlocked = true;
    fillLedSegment(LEDS_BLOCK3_SEGMENT[0], LEDS_BLOCK3_SEGMENT[1]);
    openRelay(PIN_RELAY_DSWITCHES);
}

void initDswitch()
{
    for (int i = 0; i < DSWITCH_NUM; i++)
    {
        dswitchLeds[i]
            .begin(DSWITCH_LED_PINS[i])
            .trigger(Atm_led::EVT_OFF);

        dswitchButtons[i]
            .begin(DSWITCH_PINS[i])
            .debounce(DSWITCH_DEBOUNCE_MS)
            .onPress(refreshDswitches)
            .onRelease(refreshDswitches);
    }

    dswitchControl
        .begin()
        .IF(allDswitchesPressed)
        .onChange(true, onUnlockedDswitches);
}

/**
 * Microphone functions.
 */

void onMicroThreshold(int idx, int v, int up)
{
    if (!progState.dswitchesUnlocked)
    {
        return;
    }

    // v: the last measured value (or moving average)
    // up: The direction in which the value changed (1 = up, 0 = down)

    Serial.print(F("Tap v="));
    Serial.print(v);
    Serial.print(F(" up="));
    Serial.println(up);

    bool overThreshold = v > MICRO_THRESHOLD;

    if (overThreshold && progState.microLevel < MICRO_MAX_LEVEL)
    {
        Serial.println(F("Tap+"));
        playTrack(PIN_AUDIO_TRACK_TAP);
        progState.microLevel++;
        blinkLedSegment(LEDS_BLOCK4_SEGMENT[0], LEDS_BLOCK4_SEGMENT[1]);
        refreshLedSegmentMicro();

        if (progState.microLevel == MICRO_MAX_LEVEL)
        {
            Serial.println(F("Tap unlock"));
            progState.tapUnlocked = true;
            openRelay(PIN_RELAY_TAP);
        }
    }
}

void onMicroTimer(int idx, int v, int up)
{
    if (!progState.dswitchesUnlocked)
    {
        return;
    }

    if (progState.microLevel > 0 && progState.microLevel < MICRO_MAX_LEVEL)
    {
        Serial.println(F("Tap-"));
        progState.microLevel--;
        refreshLedSegmentMicro();
    }
    else if (progState.microLevel < 0)
    {
        progState.microLevel = 0;
        refreshLedSegmentMicro();
    }
}

void initMicro()
{
    microAnalog
        .begin(MICRO_PIN, MICRO_SAMPLERATE)
        .onChange(onMicroThreshold);

    microTimer
        .begin(MICRO_TIMER_MS)
        .repeat(-1)
        .onTimer(onMicroTimer)
        .start();
}

/**
 * Relay functions.
 */

void lockRelay(int pin)
{
    if (digitalRead(pin) == HIGH)
    {
        Serial.print(F("Relay:"));
        Serial.print(pin);
        Serial.println(F(":Lock"));
    }

    digitalWrite(pin, LOW);
}

void openRelay(int pin)
{
    if (digitalRead(pin) == LOW)
    {
        Serial.print(F("Relay:"));
        Serial.print(pin);
        Serial.println(F(":Open"));
    }

    digitalWrite(pin, HIGH);
}

void initRelays()
{
    pinMode(PIN_RELAY_POTS, OUTPUT);
    pinMode(PIN_RELAY_DSWITCHES, OUTPUT);
    pinMode(PIN_RELAY_TAP, OUTPUT);

    lockRelay(PIN_RELAY_POTS);
    lockRelay(PIN_RELAY_DSWITCHES);
    lockRelay(PIN_RELAY_TAP);
}

/**
   Audio FX functions.
*/

void playTrack(byte trackPin)
{
    if (isTrackPlaying())
    {
        Serial.println(F("Skipping: Audio playing"));
        return;
    }

    Serial.print(F("Playing track on pin: "));
    Serial.println(trackPin);

    digitalWrite(trackPin, LOW);
    pinMode(trackPin, OUTPUT);
    delay(300);
    pinMode(trackPin, INPUT);
}

void initAudioPins()
{
    pinMode(PIN_AUDIO_TRACK_FILL, INPUT);
    pinMode(PIN_AUDIO_TRACK_TAP, INPUT);
    pinMode(PIN_AUDIO_ACT, INPUT);
    pinMode(PIN_AUDIO_RST, INPUT);
}

bool isTrackPlaying()
{
    return digitalRead(PIN_AUDIO_ACT) == LOW;
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

/**
 * LED strip functions.
 */

uint32_t randomColor()
{
    return Adafruit_NeoPixel::Color(random(100, 250), 0, 0);
}

void fillLedSegment(int iniIdx, int endIdx)
{
    playTrack(PIN_AUDIO_TRACK_FILL);

    for (int i = iniIdx; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, 0);
    }

    pixelStrip.show();

    for (int i = iniIdx; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, LEDS_COLOR_GREEN);
        pixelStrip.show();
        delay(LEDS_FILL_MS);
    }
}

void blinkLedSegment(int iniIdx, int endIdx)
{
    for (int i = iniIdx; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, 0);
    }

    pixelStrip.show();
    delay(LEDS_BLINK_MS);

    for (int i = iniIdx; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, LEDS_COLOR_ORANGE);
    }

    pixelStrip.show();
    delay(LEDS_BLINK_MS);

    for (int i = iniIdx; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, 0);
    }

    pixelStrip.show();
}

void refreshLedSegmentMicro()
{
    int sizeSegment = LEDS_BLOCK4_SEGMENT[1] - LEDS_BLOCK4_SEGMENT[0];
    int sizeStep = floor(((float)sizeSegment) / MICRO_MAX_LEVEL);
    int numLedsLit = sizeStep * progState.microLevel;
    int endIdx = LEDS_BLOCK4_SEGMENT[0] + numLedsLit;

    Serial.print(F("Tap LED total="));
    Serial.println(numLedsLit);

    if (progState.microLevel >= MICRO_MAX_LEVEL)
    {
        endIdx = LEDS_BLOCK4_SEGMENT[1];
    }

    for (int i = LEDS_BLOCK4_SEGMENT[0]; i < LEDS_BLOCK4_SEGMENT[1]; i++)
    {
        pixelStrip.setPixelColor(i, 0);
    }

    for (int i = LEDS_BLOCK4_SEGMENT[0]; i < endIdx; i++)
    {
        pixelStrip.setPixelColor(i, LEDS_COLOR_ORANGE);
    }

    pixelStrip.show();
}

void refreshLedSegmentPots()
{
    const int SEGMENT_SIZE = POTS_RANGE_HI - POTS_RANGE_LO;

    bool isAsc;
    int iniLed;
    int endLed;

    for (int i = 0; i < POTS_NUM; i++)
    {
        isAsc = LEDS_POTS_SEGMENTS[i][1] >
                LEDS_POTS_SEGMENTS[i][0];

        iniLed = isAsc ? LEDS_POTS_SEGMENTS[i][0] : LEDS_POTS_SEGMENTS[i][1];
        endLed = iniLed + progState.currPotValues[i];

        for (int i = iniLed; i < (iniLed + SEGMENT_SIZE); i++)
        {
            pixelStrip.setPixelColor(i, 0);
        }

        for (int i = iniLed; i < endLed; i++)
        {
            pixelStrip.setPixelColor(i, randomColor());
        }
    }

    pixelStrip.show();
}

void initLeds()
{
    pixelStrip.begin();
    pixelStrip.setBrightness(LEDS_BRIGHTNESS);
    pixelStrip.show();

    clearLeds();
    refreshLedSegmentPots();
    refreshLedSegmentMicro();
}

void clearLeds()
{
    pixelStrip.clear();
    pixelStrip.show();
}

/**
 * Configuration validation functions.
 */

void validateConfig()
{
    validatePotsKey();
    validateLedSegments();
}

void validatePotsKey()
{
    for (int i = 0; i < POTS_NUM; i++)
    {
        if (POTS_KEY[i] < POTS_RANGE_LO ||
            POTS_KEY[i] > POTS_RANGE_HI)
        {
            Serial.println(F("#####"));
            Serial.println(F("## WARNING: Invalid potentiometers key"));
            Serial.println(F("#####"));

            signalProgramError();
        }
    }
}

void validateLedSegments()
{
    const int DIFF_EXPECTED = POTS_RANGE_HI - POTS_RANGE_LO;

    bool isAsc;
    int diff;

    for (int i = 0; i < POTS_NUM; i++)
    {
        isAsc = LEDS_POTS_SEGMENTS[i][1] >
                LEDS_POTS_SEGMENTS[i][0];

        if (isAsc)
        {
            diff = LEDS_POTS_SEGMENTS[i][1] -
                   LEDS_POTS_SEGMENTS[i][0];
        }
        else
        {
            diff = LEDS_POTS_SEGMENTS[i][0] -
                   LEDS_POTS_SEGMENTS[i][1];
        }

        if (diff != DIFF_EXPECTED)
        {
            Serial.println(F("#####"));
            Serial.println(F("## WARNING: Invalid LED segments"));
            Serial.println(F("#####"));

            signalProgramError();
        }
    }
}

void signalProgramError()
{
    const int ERR_LED_DELAY_MS = 500;

    pinMode(LED_BUILTIN, OUTPUT);

    while (true)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(ERR_LED_DELAY_MS);
        digitalWrite(LED_BUILTIN, LOW);
        delay(ERR_LED_DELAY_MS);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initPots();
    initLeds();
    initAudioPins();
    resetAudio();
    initMicro();
    initRelays();
    initDswitch();
    validateConfig();

    Serial.println(F(">> Starting incubator program"));
}

void loop()
{
    automaton.run();
}