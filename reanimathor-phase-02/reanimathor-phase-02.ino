#include <CircularBuffer.h>
#include <Adafruit_NeoPixel.h>

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
const int STATE_BUFFER_SIZE = 2;

// Buffer that will contain the history of vibration states
CircularBuffer<StateSample, STATE_BUFFER_SIZE> stateBuffer;

// Minimum ratio of level changes in a sensor samples
// stream to consider that the sensor is vibrating
const float VIBRATION_LEVEL_CHANGE_RATIO = 0.05;

// Time period (ms) between state samples extracted
// from the sensor samples buffer
const unsigned long STATE_SAMPLING_PERIOD_MS = 100;

// Iteration delay (ms)
const int LOOP_WAIT_MS = 5;

const int LED_LEVELS = 10;
const int LED_BOUNCE_MS = 5000;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 60;
const uint8_t NEOPIXEL_PIN = 3;

// Initialize the NeoPixel instance
Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// NeoPixel active color
uint32_t colorActive = pixelStrip.Color(0, 255, 0);

// Timestamp (millis) of the last state sample
unsigned long lastStateMillis;

// Last observed state
byte lastState;

int currentStripLevel = 0;
unsigned long lastStripUpdateMillis;

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
void updateStateBufferIfTimePassed() {
  unsigned long now = millis();

  bool isOverflow = lastStateMillis > now;
  bool canProceed = (now - lastStateMillis) > STATE_SAMPLING_PERIOD_MS;

  if (!isOverflow && !canProceed) {
    return;
  }

  lastStateMillis = now;

  byte sensorBufferState = getSensorBufferState();

  StateSample sample;
  sample.tstamp = millis();
  sample.state = sensorBufferState;

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

/**
   Turn on the LEDs on the strip up to the given level.
*/
void setPixelsToLevel(int theLevel) {
  Serial.print("setPixelsToLevel: ");
  Serial.println(theLevel);
  Serial.flush();

  float totalPixels = (float) NEOPIXEL_NUM;
  int pixelsPerLevel = ceil(totalPixels / LED_LEVELS);
  int totalPixelsOn = pixelsPerLevel * theLevel;

  if (totalPixelsOn > NEOPIXEL_NUM) totalPixelsOn = NEOPIXEL_NUM;

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    if (i < totalPixelsOn) {
      pixelStrip.setPixelColor(i, colorActive);
    } else {
      pixelStrip.setPixelColor(i, 0, 0, 0);
    }
  }

  pixelStrip.show();
}

/**
   Turn off some LEDs after an inactivity period.
*/
void bouncePixelsIfTimePassed() {
  unsigned long now = millis();

  bool isOverflow = lastStripUpdateMillis > now;
  bool canProceed = (now - lastStripUpdateMillis) > LED_BOUNCE_MS;

  if (!isOverflow && !canProceed) {
    return;
  }

  lastStripUpdateMillis = now;

  if (currentStripLevel > 0) {
    currentStripLevel -= 1;
    setPixelsToLevel(currentStripLevel);
  }
}

/**
   Check the current state, update the last state variable if the state
   has changed and increase the LED strip level if vibrating.
*/
void checkCurrentState() {
  byte currentState = getCurrentState();

  if (currentState == STATE_UNKNOWN || currentState == lastState) {
    return;
  }

  lastState = currentState;

  if (currentState == STATE_VIBRATING) {
    lastStripUpdateMillis = millis();

    if (currentStripLevel < LED_LEVELS) {
      currentStripLevel += 1;
      setPixelsToLevel(currentStripLevel);
    }
  }

  Serial.print("##### State: ");
  printState(currentState);
  Serial.println();
  Serial.flush();
}

void setup() {
  Serial.begin(9600);

  Serial.println("Starting Reanimathor Phase 02 Program");

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  pixelStrip.begin();
  pixelStrip.setBrightness(250);
  pixelStrip.show();

  setPixelsToLevel(0);

  lastStateMillis = millis();
  lastStripUpdateMillis = millis();
}

void loop() {
  updateSensorBuffer();
  updateStateBufferIfTimePassed();
  checkCurrentState();
  bouncePixelsIfTimePassed();

  delay(LOOP_WAIT_MS);
}
