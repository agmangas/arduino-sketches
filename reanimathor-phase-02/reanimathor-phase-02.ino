#include <CircularBuffer.h>

#define STATE_STABLE 1
#define STATE_VIBRATING 2
#define STATE_UNKNOWN 3

struct sensorSample {
  unsigned long tstamp;
  int level;
};

struct stateSample {
  unsigned long tstamp;
  byte state;
};

typedef struct sensorSample SensorSample;
typedef struct stateSample StateSample;

// Pin connected to the vibration sensor
const byte SENSOR_PIN = 2;

// Size of the sensor samples buffer
const int SENSOR_BUFFER_SIZE = 100;

// Buffer that will contain the vibration sensor readings
CircularBuffer<SensorSample, SENSOR_BUFFER_SIZE> sensorBuffer;

// Size of the vibration states buffer
const int STATE_BUFFER_SIZE = 5;

// Buffer that will contain the history of vibration states
CircularBuffer<StateSample, STATE_BUFFER_SIZE> stateBuffer;

// Minimum ratio of level changes in a sensor samples
// stream to consider that the sensor is vibrating
const float VIBRATION_LEVEL_CHANGE_RATIO = 0.1;

// Time period (ms) between state samples extracted
// from the sensor samples buffer
const unsigned long STATE_SAMPLING_PERIOD_MS = 100;

// Iteration delay (ms)
const int LOOP_WAIT_MS = 5;

// Timestamp (millis) of the last state sample
unsigned long lastStateMillis;

// Last observed state
byte lastState = STATE_UNKNOWN;

/**
   Prints the string representation of the given state to the serial console.
*/
void printState(byte theState) {
  if (theState == STATE_STABLE) {
    Serial.print("STABLE");
  } else if (theState == STATE_VIBRATING) {
    Serial.print("VIBRATING");
  } else if (theState == STATE_UNKNOWN) {
    Serial.print("UNKNOWN");
  }
}

/**
   Read the sensor and push the value to the buffer.
*/
void updateSensorBuffer() {
  int sensorVal = digitalRead(SENSOR_PIN);

  SensorSample sample;
  sample.tstamp = millis();
  sample.level = sensorVal;

  sensorBuffer.push(sample);

  digitalWrite(13, sensorVal);
}

/**
   Returns the current vibration sensor status.
*/
byte getSensorBufferState() {
  if (sensorBuffer.isFull() == false) {
    return STATE_UNKNOWN;
  }

  int levelChanges = 0;
  int currentLevel = sensorBuffer.first().level;

  for (int i = 0; i < sensorBuffer.size(); i++) {
    if (currentLevel != sensorBuffer[i].level) {
      levelChanges += 1;
      currentLevel = sensorBuffer[i].level;
    }
  }

  float bufferSize = sensorBuffer.size();
  float levelChangesRatio = levelChanges / bufferSize;

  if (levelChangesRatio >= VIBRATION_LEVEL_CHANGE_RATIO) {
    return STATE_VIBRATING;
  } else {
    return STATE_STABLE;
  }
}

/**
   Reads the current sensor state and updates the state buffer.
*/
void updateStateBuffer() {
  unsigned long now = millis();

  bool isOverflow = lastStateMillis > now;
  bool canProceed = (now - lastStateMillis) > STATE_SAMPLING_PERIOD_MS;

  if (isOverflow || canProceed) {
    lastStateMillis = now;
  } else {
    return;
  }

  byte sensorBufferState = getSensorBufferState();

  StateSample sample;
  sample.tstamp = millis();
  sample.state = sensorBufferState;

  Serial.print("updateStateBuffer: ");
  printState(sensorBufferState);
  Serial.println();
  Serial.flush();

  stateBuffer.push(sample);
}

/**
   Returns the current state extracted from the state buffer.
*/
byte getCurrentState() {
  if (stateBuffer.isFull() == false) {
    return STATE_UNKNOWN;
  }

  byte firstState = stateBuffer.first().state;

  for (int i = 0; i < stateBuffer.size(); i++) {
    if (firstState != stateBuffer[i].state) {
      return STATE_UNKNOWN;
    }
  }

  return firstState;
}

void setup() {
  Serial.begin(9600);

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  lastStateMillis = millis();
}

void loop() {
  updateSensorBuffer();
  updateStateBuffer();

  byte currentState = getCurrentState();

  if (currentState != STATE_UNKNOWN && currentState != lastState) {
    lastState = currentState;

    Serial.print("##### Update: ");
    printState(currentState);
    Serial.println();
    Serial.flush();
  }

  delay(LOOP_WAIT_MS);
}
