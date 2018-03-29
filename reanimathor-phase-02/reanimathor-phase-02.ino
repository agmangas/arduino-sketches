#include <CircularBuffer.h>

#define STATE_STABLE 1
#define STATE_VIBRATING 2

// Define the struct to contain vibration sensor reading samples
struct sensorSample {
  unsigned long tstamp;
  int level;
};

typedef struct sensorSample SensorSample;

// Pin connected to the vibration sensor
const byte SENSOR_PIN = 2;

// Buffer that will contain the vibration sensor readings
const int BUFFER_SIZE = 60;
CircularBuffer<SensorSample, BUFFER_SIZE> sensorBuffer;

// Vibration ratio threshold
const float VIBRATION_RATIO = 0.7;

/**
   Read the sensor and push the value to the buffer.
*/
void readSensor() {
  int sensorVal = digitalRead(SENSOR_PIN);

  SensorSample sample;
  sample.tstamp = millis();
  sample.level = sensorVal;

  sensorBuffer.push(sample);

  digitalWrite(13, sensorVal);
}

/**
   Returns the current state (HIGH, LOW, VIBRATING).
*/
byte getCurrentState() {
  int counterHi = 0;
  int counterLo = 0;

  for (int i = 0; i < sensorBuffer.size(); i++) {
    if (sensorBuffer[i].level == HIGH) {
      counterHi += 1;
    } else {
      counterLo += 1;
    }
  }

  float bufferSize = sensorBuffer.size();

  if (counterHi / bufferSize >= VIBRATION_RATIO) {
    return STATE_STABLE;
  } else if (counterLo / bufferSize >= VIBRATION_RATIO) {
    return STATE_STABLE;
  } else {
    return STATE_VIBRATING;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);
}

void loop() {
  readSensor();

  byte currentState = getCurrentState();

  if (currentState == STATE_STABLE) {
    Serial.println("STABLE");
  } else if (currentState == STATE_VIBRATING) {
    Serial.println("VIBRATING");
  }
}
