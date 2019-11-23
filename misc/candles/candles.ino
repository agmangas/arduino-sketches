#include <CircularBuffer.h>

const bool PRINT_CANDLE_VOLTS = true;
const int SAMPLE_WINDOW_MS = 50;
const int NUM_CANDLES = 4;
const int VOLTS_BUF_SIZE = 25;
const double PEAK_PEAK_THRESHOLD_VOLTS = 0.5;
const int PEAK_PEAK_THRESHOLD_NUM = 5;

const int CANDLE_MIC_PINS[NUM_CANDLES] = {
    A0, A1, A2, A3};

const int CANDLE_RELAY_PINS[NUM_CANDLES] = {
    2, 3, 4, 5};

CircularBuffer<double, VOLTS_BUF_SIZE> voltsBufs[NUM_CANDLES];

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
            openRelay(CANDLE_RELAY_PINS[i]);
        }
    }
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
}

void setup()
{
    Serial.begin(9600);
    initRelays();
    Serial.println(F(">> Starting candles program"));
}

void loop()
{
    sampleMics();
    checkMics();
}