#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Relay.
 */

const int RELAY_PIN = A3;

/**
 * Audio FX.
 */

const byte PIN_AUDIO_RST = 6;
const byte PIN_AUDIO_ACT = 5;
const byte PIN_TRACK_GOAL = A2;
const byte PIN_TRACK_FAIL = A1;
const byte PIN_TRACK_VICTORY = A0;

/**
 * Proximity sensors.
 */

const int SENSOR_NUM = 3;
const int SENSOR_UPDATE_DELAY_MS = 1000;

const int SENSOR_PINS[SENSOR_NUM] = {
    2, 3, 4};

Atm_button sensorButtons[SENSOR_NUM];

/**
 * LEDs.
 */

const int LED_BRIGHTNESS = 200;

const int LED_SENSOR_NUMS[SENSOR_NUM] = {
    60, 60, 60};

const int LED_SENSOR_PINS[SENSOR_NUM] = {
    7, 8, 9};

Adafruit_NeoPixel sensorLedStrips[SENSOR_NUM] = {
    Adafruit_NeoPixel(
        LED_SENSOR_NUMS[0],
        LED_SENSOR_PINS[0],
        NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(
        LED_SENSOR_NUMS[1],
        LED_SENSOR_PINS[1],
        NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(
        LED_SENSOR_NUMS[2],
        LED_SENSOR_PINS[2],
        NEO_GRB + NEO_KHZ800)};

const uint32_t LED_SENSOR_COLORS[SENSOR_NUM] = {
    Adafruit_NeoPixel::Color(250, 0, 0),
    Adafruit_NeoPixel::Color(0, 250, 0),
    Adafruit_NeoPixel::Color(0, 0, 250)};

const int LED_SENSOR_ERROR_DELAY_MS = 100;
const int LED_SENSOR_ERROR_ITERS = 10;
const int LED_SENSOR_GOAL_DELAY_MS = 1000;
const uint32_t LED_SENSOR_GOAL_COLOR = Adafruit_NeoPixel::Color(255, 255, 0);

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
    0, 1, 2, 0, 1, 2, 0, 1, 2, 0};

int sensorsConfig[SENSOR_NUM];
int results[RESULTS_SIZE];

typedef struct programState
{
    unsigned long updateMillis;
    int *sensorsConfig;
    int *results;
    bool isVictory;
} ProgramState;

ProgramState progState = {
    .updateMillis = 0,
    .sensorsConfig = sensorsConfig,
    .results = results,
    .isVictory = false};

/**
 * Proximity sensor functions.
 */

void onVictory()
{
    playTrack(PIN_TRACK_VICTORY);
    progState.isVictory = true;
    openRelay();
    Serial.println(F("Victory"));
}

void onSensorPress(int idx, int v, int up)
{
    if (progState.isVictory)
    {
        return;
    }

    int result = progState.sensorsConfig[idx];

    Serial.print(F("Sensor:"));
    Serial.print(idx);
    Serial.print(F(" Result:"));
    Serial.println(result);

    addResult(result);

    if (isResultsValid())
    {
        onVictory();
        return;
    }

    if (isResultsError())
    {
        playTrack(PIN_TRACK_FAIL);
        emptyResults();
        showErrorSensorLedsPattern();
    }
    else
    {
        playTrack(PIN_TRACK_GOAL);
        showGoalLeds(idx);
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

void clearSensorLeds()
{
    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].clear();
        sensorLedStrips[i].show();
    }
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

void showRedSensorLeds()
{
    const uint32_t red = Adafruit_NeoPixel::Color(255, 0, 0);

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        for (int j = 0; j < sensorLedStrips[i].numPixels(); j++)
        {
            sensorLedStrips[i].setPixelColor(j, red);
        }
    }

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].show();
    }
}

void showErrorSensorLedsPattern()
{
    for (int i = 0; i < LED_SENSOR_ERROR_ITERS; i++)
    {
        clearSensorLeds();
        delay(LED_SENSOR_ERROR_DELAY_MS);
        showRedSensorLeds();
        delay(LED_SENSOR_ERROR_DELAY_MS);
    }

    clearSensorLeds();
}

void showVictoryLedsPattern()
{
    const int delayMs = 20;
    const int referenceIdx = 0;
    const uint32_t color = Adafruit_NeoPixel::Color(200, 200, 200);

    clearSensorLeds();

    for (int j = 0; j < sensorLedStrips[referenceIdx].numPixels(); j++)
    {
        for (int i = 0; i < SENSOR_NUM; i++)
        {
            sensorLedStrips[i].setPixelColor(j, color);
            sensorLedStrips[i].show();
        }

        delay(delayMs);
    }
}

void showGoalLeds(int goalIdx)
{
    uint32_t color;

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        color = i == goalIdx ? LED_SENSOR_GOAL_COLOR : 0;

        for (int j = 0; j < sensorLedStrips[i].numPixels(); j++)
        {
            sensorLedStrips[i].setPixelColor(j, color);
        }
    }

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].show();
    }

    delay(LED_SENSOR_GOAL_DELAY_MS);
    clearSensorLeds();
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
    clearSensorLeds();
    delay(SENSOR_UPDATE_DELAY_MS);
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

bool isResultsValid()
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
 * Audio FX functions.
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
    pinMode(PIN_TRACK_GOAL, INPUT);
    pinMode(PIN_TRACK_FAIL, INPUT);
    pinMode(PIN_TRACK_VICTORY, INPUT);
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

    initLeds();
    initSensorButtons();
    updateSensorsConfig();
    emptyResults();
    initRelay();
    initAudioPins();
    resetAudio();

    Serial.println(F(">> Starting quidditch program"));
}

void loop()
{
    automaton.run();

    if (progState.isVictory)
    {
        showVictoryLedsPattern();
    }
    else
    {
        checkSensorsConfig();
    }
}
