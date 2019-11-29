#include <CircularBuffer.h>

const bool PRINT_CANDLE_VOLTS = false;
const int NUM_CANDLES = 4;
const int VOLTS_BUF_SIZE = 25;
const int SAMPLE_WINDOW_MS = 30;
const double PEAK_PEAK_THRESHOLD_VOLTS = 0.8;
const int PEAK_PEAK_THRESHOLD_NUM = 3;

const int CANDLE_MIC_PINS[NUM_CANDLES] = {
    A0, A1, A2, A3};

const int CANDLE_RELAY_PINS[NUM_CANDLES] = {
    2, 3, 4, 5};

const int BOX_RELAY_PIN = 6;

bool candleRelayStates[NUM_CANDLES] = {
    false, false, false, false};

const int CANDLE_ACTIVATION_KEY[NUM_CANDLES] = {
    2, 3, 0, 1};

int candleActivationOrder[NUM_CANDLES];

CircularBuffer<double, VOLTS_BUF_SIZE> voltsBufs[NUM_CANDLES];

void emptyActivation()
{
    if (isActivationEmpty() == false)
    {
        Serial.println(F("Clearing activation buffer"));
    }

    for (int i = 0; i < NUM_CANDLES; i++)
    {
        candleActivationOrder[i] = -1;
    }
}

void resetIfAllOut()
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (candleRelayStates[i] == true)
        {
            return;
        }
    }

    emptyActivation();
    lockRelay(BOX_RELAY_PIN);
}

bool isActivationFull()
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (candleActivationOrder[i] == -1)
        {
            return false;
        }
    }

    return true;
}

bool isActivationEmpty()
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (candleActivationOrder[i] != -1)
        {
            return false;
        }
    }

    return true;
}

bool isValidActivation()
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (CANDLE_ACTIVATION_KEY[i] != candleActivationOrder[i])
        {
            return false;
        }
    }

    return true;
}

void pushActivation(int idx)
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (candleActivationOrder[i] == -1)
        {
            Serial.print(F("Activation: "));
            Serial.println(idx);
            candleActivationOrder[i] = idx;
            break;
        }
    }

    if (isActivationFull() && isValidActivation())
    {
        Serial.println(F("Opening box relay"));
        openRelay(BOX_RELAY_PIN);
    }
}

bool isMicActivated(int micIdx)
{
    if (voltsBufs[micIdx].size() < PEAK_PEAK_THRESHOLD_NUM)
    {
        return false;
    }

    int activeCounter = 0;

    for (int i = voltsBufs[micIdx].size() - 1; i >= 0; i--)
    {
        if (voltsBufs[micIdx][i] >= PEAK_PEAK_THRESHOLD_VOLTS)
        {
            activeCounter++;
        }

        if (activeCounter >= PEAK_PEAK_THRESHOLD_NUM)
        {
            return true;
        }
    }

    return false;
}

void checkMics()
{
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        if (isMicActivated(i))
        {
            if (candleRelayStates[i] == true)
            {
                lockRelay(CANDLE_RELAY_PINS[i]);
                candleRelayStates[i] = false;
            }
            else
            {
                openRelay(CANDLE_RELAY_PINS[i]);
                candleRelayStates[i] = true;
                pushActivation(i);
            }

            voltsBufs[i].clear();
        }
    }

    resetIfAllOut();
}

void sampleMics()
{
    double volts;

    for (int i = 0; i < NUM_CANDLES; i++)
    {
        volts = readPeakToPeakVolts(CANDLE_MIC_PINS[i]);
        voltsBufs[i].push(volts);

        if (PRINT_CANDLE_VOLTS && i < (NUM_CANDLES - 1))
        {
            Serial.print(volts);
            Serial.print("\t");
        }
        else if (PRINT_CANDLE_VOLTS)
        {
            Serial.println(volts);
        }
    }
}

double readPeakToPeakVolts(int pin)
{
    unsigned long startMillis = millis();

    int signalMax = 0;
    int signalMin = 1024;
    int sample;

    while ((millis() - startMillis) < SAMPLE_WINDOW_MS)
    {
        sample = analogRead(pin);

        if (sample >= 1024)
        {
            continue;
        }

        if (sample > signalMax)
        {
            signalMax = sample;
        }
        else if (sample < signalMin)
        {
            signalMin = sample;
        }
    }

    int peakToPeak = signalMax - signalMin;
    double volts = (peakToPeak * 5.0) / 1024;

    return volts;
}

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
    for (int i = 0; i < NUM_CANDLES; i++)
    {
        pinMode(CANDLE_RELAY_PINS[i], OUTPUT);
        lockRelay(CANDLE_RELAY_PINS[i]);
    }

    pinMode(BOX_RELAY_PIN, OUTPUT);
    lockRelay(BOX_RELAY_PIN);
}

void setup()
{
    Serial.begin(9600);

    emptyActivation();
    initRelays();

    Serial.println(F(">> Starting candles program"));
}

void loop()
{
    sampleMics();
    checkMics();
}
