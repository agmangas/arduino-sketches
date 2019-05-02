#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Proximity sensors.
 */

const int SENSOR_NUM = 3;

const int SENSOR_PINS[SENSOR_NUM] = {
    2, 3, 4};

Atm_button sensorButtons[SENSOR_NUM];

/**
 * LEDs.
 */

const int LED_BRIGHTNESS = 200;

const int LED_SENSOR_NUMS[SENSOR_NUM] = {
    30, 30, 30};

const int LED_SENSOR_PINS[SENSOR_NUM] = {
    7, 8, 9};

Adafruit_NeoPixel sensorLedStrips[SENSOR_NUM];

const uint32_t LED_SENSOR_COLORS[SENSOR_NUM] = {
    Adafruit_NeoPixel::Color(250, 0, 0),
    Adafruit_NeoPixel::Color(0, 250, 0),
    Adafruit_NeoPixel::Color(0, 0, 250)};

const int LED_RESULT_NUM = 30;
const int LED_RESULT_PIN = 10;

Adafruit_NeoPixel resultLedStrip = Adafruit_NeoPixel(
    LED_RESULT_NUM,
    LED_RESULT_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

const unsigned long ACTIVE_MILLIS = 6000;
const int RESULTS_SIZE = 10;

const int RESULTS_KEY[RESULTS_SIZE] = {
    0, 1, 1, 2, 1, 0, 1, 0, 0, 2};

int sensorsConfig[SENSOR_NUM];
int results[RESULTS_SIZE];

typedef struct programState
{
    unsigned long updateMillis;
    int *sensorsConfig;
    int *results;
} ProgramState;

ProgramState progState = {
    .updateMillis = 0,
    .sensorsConfig = sensorsConfig,
    .results = results};

/**
 * Proximity sensor functions.
 */

void onSensorPress(int idx, int v, int up)
{
    int result = progState.sensorsConfig[idx];

    Serial.print(F("Sensor:"));
    Serial.print(idx);
    Serial.print(F(" Result:"));
    Serial.println(result);

    addResult(result);

    if (isResultsComplete())
    {
        Serial.println(F("Complete"));
        return;
    }

    if (isResultsError())
    {
        emptyResults();
    }

    showResults();
    updateSensorsConfig();
}

void initSensorButtons()
{
    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorButtons[i]
            .begin(SENSOR_PINS[i])
            .onPress(onSensorPress, i);
    }
}

/**
 * LED functions.
 */

void initLeds()
{
    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i] = Adafruit_NeoPixel(
            LED_SENSOR_NUMS[i],
            LED_SENSOR_PINS[i],
            NEO_GRB + NEO_KHZ800);

        sensorLedStrips[i].begin();
        sensorLedStrips[i].setBrightness(LED_BRIGHTNESS);
        sensorLedStrips[i].clear();
        sensorLedStrips[i].show();
    }

    resultLedStrip.begin();
    resultLedStrip.setBrightness(LED_BRIGHTNESS);
    resultLedStrip.clear();
    resultLedStrip.show();
}

void showSensorsConfig()
{
    for (int i = 0; i < SENSOR_NUM; i++)
    {
        int colorIdx = progState.sensorsConfig[i];
        uint32_t color = LED_SENSOR_COLORS[colorIdx];

        for (int j = 0; j < LED_SENSOR_NUMS[i]; j++)
        {
            sensorLedStrips[i].setPixelColor(j, color);
        }
    }

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].show();
    }
}

int getResultLedsPerLevel()
{
    int num = floor(((float)LED_RESULT_NUM) / RESULTS_SIZE);
    num = num < 1 ? 1 : num;

    return num;
}

void showResults()
{
    int numPerLevel = getResultLedsPerLevel();
    int currIdx = 0;

    resultLedStrip.clear();
    resultLedStrip.show();

    for (int i = 0; i < RESULTS_SIZE; i++)
    {
        int result = progState.results[i];

        if (result == -1)
        {
            return;
        }

        uint32_t color = LED_SENSOR_COLORS[result];

        for (int j = 0; j < numPerLevel; j++)
        {
            resultLedStrip.setPixelColor(currIdx, color);
            resultLedStrip.show();

            currIdx++;
        }
    }
}

/**
 * Game state functions.
 */

bool mustUpdateSensorConfig()
{
    if (progState.updateMillis == 0)
    {
        Serial.println(F("Update: First"));
        return true;
    }

    unsigned long now = millis();

    if (now < progState.updateMillis)
    {
        Serial.println(F("Update: Overflow"));
        return true;
    }

    unsigned long diff = now - progState.updateMillis;

    if (diff > ACTIVE_MILLIS)
    {
        Serial.println(F("Update: Expired"));
        return true;
    }

    return false;
}

void updateSensorsConfig()
{
    progState.updateMillis = millis();
    randomSensorsConfig();
    showSensorsConfig();
}

void checkSensorsConfig()
{
    if (!mustUpdateSensorConfig())
    {
        return;
    }

    updateSensorsConfig();
}

void randomSensorsConfig()
{
    int pivot = random(0, SENSOR_NUM);

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        progState.sensorsConfig[i] = (pivot + i) % SENSOR_NUM;
    }
}

void emptyResults()
{
    for (int i = 0; i < RESULTS_SIZE; i++)
    {
        progState.results[i] = -1;
    }
}

bool addResult(int val)
{
    if (val < 0 || val >= SENSOR_NUM)
    {
        return false;
    }

    for (int i = 0; i < RESULTS_SIZE; i++)
    {
        if (progState.results[i] == -1)
        {
            progState.results[i] = val;
            return true;
        }
    }

    return false;
}

bool isResultsError()
{
    for (int i = 0; i < RESULTS_SIZE; i++)
    {
        if (progState.results[i] == -1)
        {
            return false;
        }
        else if (progState.results[i] != RESULTS_KEY[i])
        {
            return true;
        }
    }

    return false;
}

bool isResultsComplete()
{
    for (int i = 0; i < RESULTS_SIZE; i++)
    {
        if (progState.results[i] == -1 ||
            progState.results[i] != RESULTS_KEY[i])
        {
            return false;
        }
    }

    return true;
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initLeds();
    initSensorButtons();
    updateSensorsConfig();
    emptyResults();

    Serial.println(F(">> Starting quidditch program"));
}

void loop()
{
    automaton.run();
    checkSensorsConfig();
}
