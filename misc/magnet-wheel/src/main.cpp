#include <Automaton.h>

const int SENSOR_NUM = 3;

const int SENSOR_PINS[SENSOR_NUM] = {
    2, 3, 4
};

Atm_button sensorButtons[SENSOR_NUM];
Atm_controller sensorController;

const int RELAY_PIN = 11;
const unsigned long RELAY_CLOSE_DELAY_MS = 5000;

struct ProgramState {
    unsigned millisSensorsOff;
};

ProgramState progState;

void initState()
{
    progState.millisSensorsOff = 0;
}

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

void checkRelay()
{
    if (progState.millisSensorsOff == 0) {
        return;
    }

    unsigned long now = millis();

    unsigned long millisLock = progState.millisSensorsOff
        + RELAY_CLOSE_DELAY_MS;

    if (now >= millisLock) {
        lockRelay();
        progState.millisSensorsOff = 0;
    }
}

bool allSensorsActive(int)
{
    for (int i = 0; i < SENSOR_NUM; i++) {
        if (sensorButtons[i].state() == Atm_button::BTN_RELEASE) {
            return false;
        }
    }

    return true;
}

void onSensorPress(int idx, int v, int up)
{
    Serial.print("Sensor ON #");
    Serial.println(idx);
}

void onControllerUp(int, int, int)
{
    Serial.println(F("Sensor controller up"));
    progState.millisSensorsOff = 0;
    openRelay();
}

void onControllerDown(int, int, int)
{
    Serial.println(F("Sensor controller down"));
    progState.millisSensorsOff = millis();
}

void initSensors()
{
    for (int i = 0; i < SENSOR_NUM; i++) {
        sensorButtons[i]
            .begin(SENSOR_PINS[i])
            .onPress(onSensorPress, i);
    }

    sensorController
        .begin()
        .IF(allSensorsActive)
        .onChange(true, onControllerUp)
        .onChange(false, onControllerDown);
}

void setup()
{
    Serial.begin(9600);

    initState();
    initSensors();
    initRelay();

    Serial.println(">> Hall effect proof of concept");
}

void loop()
{
    automaton.run();
    checkRelay();
}
