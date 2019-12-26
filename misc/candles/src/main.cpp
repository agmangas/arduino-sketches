#include <Arduino.h>
#include <CircularBuffer.h>

/**
 * Set to true to print voltage values from the mics 
 * in an Arduino Serial Plotter compatible format.
 */
const bool PRINT_CANDLE_VOLTS = true;

/**
 * Mics activation sensitivity configuration.
 */
const int SAMPLE_WINDOW_MS = 30;
const double PEAK_PEAK_THRESHOLD_VOLTS = 0.8;
const int PEAK_PEAK_THRESHOLD_NUM = 3;

const int NUM_CANDLES = 4;

const int CANDLE_MIC_PINS[NUM_CANDLES] = {
    A0, A1, A2, A3
};

const int BOX_RELAY_PIN = 6;

bool candleActivationStates[NUM_CANDLES] = {
    false, false, false, false
};

const int CANDLE_ACTIVATION_KEY[NUM_CANDLES] = {
    2, 3, 0, 1
};

int candleActivationOrder[NUM_CANDLES];

const int VOLTS_BUF_SIZE = 25;

CircularBuffer<int, NUM_CANDLES> candleActivationBuf;
CircularBuffer<double, VOLTS_BUF_SIZE> voltsBufs[NUM_CANDLES];

void lockRelay(int pin)
{
    digitalWrite(pin, LOW);
}

void openRelay(int pin)
{
    digitalWrite(pin, HIGH);
}

void initRelays()
{
    pinMode(BOX_RELAY_PIN, OUTPUT);
    lockRelay(BOX_RELAY_PIN);
}

void initLeds()
{
}

void refreshLeds()
{
}

void showLedStartEffect()
{
}

void clearActivation()
{
    if (!candleActivationBuf.isEmpty()) {
        Serial.println(F("Clearing activation buffer"));
    }

    candleActivationBuf.clear();
}

void resetIfAllOut()
{
    for (int i = 0; i < NUM_CANDLES; i++) {
        if (candleActivationStates[i] == true) {
            return;
        }
    }

    clearActivation();
    lockRelay(BOX_RELAY_PIN);
}

bool isValidActivation()
{
    if (candleActivationBuf.size() < NUM_CANDLES) {
        return false;
    }

    for (int i = 0; i < NUM_CANDLES; i++) {
        if (candleActivationBuf[i] != CANDLE_ACTIVATION_KEY[i]) {
            return false;
        }
    }

    return true;
}

void pushActivation(int idx)
{
    if (candleActivationBuf.available() > 0) {
        Serial.print(F("Activation: "));
        Serial.println(idx);
        candleActivationBuf.push(idx);
    }

    if (candleActivationBuf.available() == 0 && isValidActivation()) {
        Serial.println(F("Opening box relay"));
        openRelay(BOX_RELAY_PIN);
    }
}

bool isMicActivated(int micIdx)
{
    if (voltsBufs[micIdx].size() < PEAK_PEAK_THRESHOLD_NUM) {
        return false;
    }

    int activeCounter = 0;

    for (int i = voltsBufs[micIdx].size() - 1; i >= 0; i--) {
        if (voltsBufs[micIdx][i] >= PEAK_PEAK_THRESHOLD_VOLTS) {
            activeCounter++;
        }

        if (activeCounter >= PEAK_PEAK_THRESHOLD_NUM) {
            return true;
        }
    }

    return false;
}

void checkMics()
{
    for (int i = 0; i < NUM_CANDLES; i++) {
        if (!isMicActivated(i)) {
            continue;
        }

        if (candleActivationStates[i] == true) {
            candleActivationStates[i] = false;
        } else {
            candleActivationStates[i] = true;
            pushActivation(i);
        }

        voltsBufs[i].clear();
    }

    resetIfAllOut();
}

double readPeakToPeakVolts(int pin)
{
    const int maxAnalogVal = 1024;

    unsigned long startMillis = millis();

    int signalMax = 0;
    int signalMin = 1024;
    int sample;

    while ((millis() - startMillis) < SAMPLE_WINDOW_MS) {
        sample = analogRead(pin);

        if (sample >= maxAnalogVal) {
            continue;
        }

        if (sample > signalMax) {
            signalMax = sample;
        } else if (sample < signalMin) {
            signalMin = sample;
        }
    }

    int peakToPeak = signalMax - signalMin;
    double volts = (peakToPeak * 5.0) / 1024;

    return volts;
}

void sampleMics()
{
    double volts;

    for (int i = 0; i < NUM_CANDLES; i++) {
        volts = readPeakToPeakVolts(CANDLE_MIC_PINS[i]);
        voltsBufs[i].push(volts);

        if (PRINT_CANDLE_VOLTS && i < (NUM_CANDLES - 1)) {
            Serial.print(volts);
            Serial.print("\t");
        } else if (PRINT_CANDLE_VOLTS) {
            Serial.println(volts);
        }
    }
}

void setup()
{
    Serial.begin(9600);

    clearActivation();
    initRelays();
    initLeds();

    Serial.println(F(">> Starting candles program"));

    showLedStartEffect();
}

void loop()
{
    sampleMics();
    checkMics();
    refreshLeds();
}
