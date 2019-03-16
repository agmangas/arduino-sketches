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
    5, 10, 15, 20};

const int POTS_BOUNCE_MS = 1000;

/**
 * Relays.
 * LEDs relay: Open on valid LED combination.
 * Digital switches relay: Open on valid digital switches combination.
 * Microphone relay: Open on microphone max level.
 */

const int PIN_RELAY_LEDS = 7;
const int PIN_RELAY_DSWITCHES = 8;
const int PIN_RELAY_MICRO = 9;

/**
 * Digital switch locks.
 */

const int DSWITCH_NUM = 3;
const int DSWITCH_DEBOUNCE_MS = 1000;
const int DSWITCH_PINS[DSWITCH_NUM] = {5, 12, A0};
const int DSWITCH_LED_PINS[DSWITCH_NUM] = {2, 3, 4};

Atm_button dswitchButtons[DSWITCH_NUM];
Atm_led dswitchLeds[DSWITCH_NUM];

/**
 * Audio FX.
 */

const int NUM_TRACKS = 2;
const int AUDIO_TRACK_PINS[NUM_TRACKS] = {A3, A2};
const int PIN_AUDIO_ACT = 11;
const int PIN_AUDIO_RST = 10;

/**
 * Microphone.
 */

const int MICRO_PIN = A1;
const int MICRO_SAMPLERATE = 50;
const int MICRO_BUF_SIZE = 4;
const int MICRO_MAX_LEVEL = 20;
const int MICRO_TIMER_MS = 1000;
const int MICRO_THRESHOLDS_NUM = 1;
const uint16_t MICRO_THRESHOLDS[MICRO_THRESHOLDS_NUM] = {600};

Atm_comparator microComp;
Atm_timer microTimer;
uint16_t microBuf[MICRO_BUF_SIZE];

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

// Lower value is always the lower limit (i.e. the inclusive index).

const int LEDS_POTS_SEGMENTS[POTS_NUM][2] = {
    {0, 20},
    {40, 20},
    {40, 60},
    {80, 60}};

const int LEDS_BLOCK2_SEGMENT[2] = {80, 110};
const int LEDS_BLOCK3_SEGMENT[2] = {110, 140};
const int LEDS_BLOCK4_SEGMENT[2] = {140, 188};

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(LEDS_NUM, LEDS_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

int currPotValues[POTS_NUM];

typedef struct programState
{
    bool potsUnlocked;
    int *currPotValues;
    unsigned long millisValidPots;
    int microLevel;
} ProgramState;

ProgramState progState = {
    .potsUnlocked = false,
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
        if (progState.currPotValues[i] != POTS_KEY[i])
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
}

void onPotChange(int idx, int v, int up)
{
    Serial.print(F("Pot["));
    Serial.print(idx);
    Serial.print(F("]:"));
    Serial.println(v);

    progState.currPotValues[idx] = v;

    refreshLedPotSegments();
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
    for (int i = 0; i < DSWITCH_NUM; i++)
    {
        if (dswitchButtons[i].state() == Atm_button::PRESSED)
        {
            dswitchLeds[i].trigger(Atm_led::EVT_ON);
        }
        else
        {
            dswitchLeds[i].trigger(Atm_led::EVT_OFF);
        }
    }
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
}

/**
 * Microphone functions.
 */

void onMicroThreshold(int idx, int v, int up)
{
    // v: The index of the threshold that was crossed
    // up: The direction in which the threshold was crossed (1 = up, 0 = down)

    Serial.println(F("onMicroThreshold idx="));
    Serial.println(v);
    Serial.println(F(" dir="));
    Serial.println(up);

    if (up == 1 && progState.microLevel < MICRO_MAX_LEVEL)
    {
        Serial.println(F("Micro++"));
        progState.microLevel++;
    }
}

void onMicroTimer(int idx, int v, int up)
{
    if (progState.microLevel > 0)
    {
        Serial.println(F("Micro--"));
        progState.microLevel--;
    }
    else if (progState.microLevel < 0)
    {
        progState.microLevel = 0;
    }
}

void initMicro()
{
    microComp
        .begin(MICRO_PIN, MICRO_SAMPLERATE)
        .threshold(MICRO_THRESHOLDS, MICRO_THRESHOLDS_NUM)
        .average(microBuf, MICRO_BUF_SIZE)
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
    pinMode(PIN_RELAY_LEDS, OUTPUT);
    pinMode(PIN_RELAY_DSWITCHES, OUTPUT);
    pinMode(PIN_RELAY_MICRO, OUTPUT);

    lockRelay(PIN_RELAY_LEDS);
    lockRelay(PIN_RELAY_DSWITCHES);
    lockRelay(PIN_RELAY_MICRO);
}

/**
   Audio FX functions.
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

void initAudioPins()
{
    for (int i = 0; i < NUM_TRACKS; i++)
    {
        pinMode(AUDIO_TRACK_PINS[i], INPUT);
    }

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

void refreshLedPotSegments()
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
    refreshLedPotSegments();
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