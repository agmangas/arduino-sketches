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

const uint32_t LED_SENSOR_COLOR = Adafruit_NeoPixel::Color(220, 40, 0);

const int LED_SENSOR_PINS[SENSOR_NUM] = {
    7, 8, 9};

const int LED_SENSOR_NUMS[SENSOR_NUM] = {
    30, 30, 30};

const int LED_RESULT_PIN = 10;
const int LED_RESULT_NUM = 10;

Adafruit_NeoPixel sensorLedStrips[SENSOR_NUM];

Adafruit_NeoPixel resultLedStrip = Adafruit_NeoPixel(
    LED_RESULT_NUM,
    LED_RESULT_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

const unsigned long ACTIVE_MILLIS_LO = 5000;
const unsigned long ACTIVE_MILLIS_MD = 15000;
const unsigned long ACTIVE_MILLIS_HI = 30000;

const int TARGET_SCORE = 6;

typedef struct programState
{
    int score;
    int activeSensor;
    unsigned long activeSensorMillis;
} ProgramState;

ProgramState progState = {
    .score = 0,
    .activeSensor = -1,
    .activeSensorMillis = 0};

/**
 * Proximity sensor functions.
 */

void onSensorPress(int idx, int v, int up)
{
    Serial.print(F("S:"));
    Serial.println(idx);

    if (progState.activeSensor != idx)
    {
        return;
    }

    Serial.println(F("Goal"));
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

void showActiveSensorLeds()
{
    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].clear();
    }

    if (progState.activeSensor >= 0 &&
        progState.activeSensor < SENSOR_NUM)
    {
        int numLeds = LED_SENSOR_NUMS[progState.activeSensor];

        for (int i = 0; i < numLeds; i++)
        {
            sensorLedStrips[i].setPixelColor(i, LED_SENSOR_COLOR);
        }
    }

    for (int i = 0; i < SENSOR_NUM; i++)
    {
        sensorLedStrips[i].show();
    }
}

/**
 * Game state functions.
 */

bool mustUpdateActiveSensor()
{
    if (progState.activeSensor == -1)
    {
        return true;
    }

    unsigned long now = millis();

    if (now < progState.activeSensorMillis)
    {
        Serial.println(F("Active sensor clock overflow"));
        return true;
    }

    unsigned long diff = now - progState.activeSensorMillis;
    unsigned long maxDiff = getActiveMillisForScore(progState.score);

    if (diff > maxDiff)
    {
        Serial.println(F("Expired"));
        return true;
    }

    return false;
}

void updateActiveSensor()
{
    if (!mustUpdateActiveSensor())
    {
        return;
    }

    int randSensor = random(0, SENSOR_NUM);

    Serial.print(F("Active: "));
    Serial.println(randSensor);

    progState.activeSensor = randSensor;
    progState.activeSensorMillis = millis();

    showActiveSensorLeds();
}

unsigned long getActiveMillisForScore(int score)
{
    const float RATIO_LO = 0.33;
    const float RATIO_HI = 0.66;

    float scoreRatio = ((float)score) / TARGET_SCORE;

    if (scoreRatio <= RATIO_LO)
    {
        return ACTIVE_MILLIS_HI;
    }
    else if (scoreRatio > RATIO_LO && scoreRatio <= RATIO_HI)
    {
        return ACTIVE_MILLIS_MD;
    }
    else
    {
        return ACTIVE_MILLIS_LO;
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initLeds();
    initSensorButtons();

    Serial.println(F(">> Starting quidditch program"));
}

void loop()
{
    automaton.run();
    updateActiveSensor();
}
