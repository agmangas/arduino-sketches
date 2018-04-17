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
const byte SENSOR_PIN = A0;

// Pin connected to the physical activation switch
const byte ACTIVATION_PIN = 10;

// Pin connected to the activation signal LED
const byte ACTIVATION_LED_PIN = 9;

// Size of the sensor samples buffer
const int SENSOR_BUFFER_SIZE = 100;

// Buffer that will contain the vibration sensor readings
CircularBuffer<SensorSample, SENSOR_BUFFER_SIZE> sensorBuffer;

// Size of the vibration states buffer
const int STATE_BUFFER_SIZE = 1;

// Buffer that will contain the history of vibration states
CircularBuffer<StateSample, STATE_BUFFER_SIZE> stateBuffer;

// Time period (ms) between state samples extracted
// from the sensor samples buffer
const unsigned long STATE_SAMPLING_PERIOD_MS = 5;

// Sensor analog level threshold
const int SENSOR_ANALOG_LEVEL_THRESHOLD = 500;

// Iteration delay (ms)
const int LOOP_WAIT_MS = 1;

// Total number of LED strip partitions (levels)
const int LED_LEVELS = 30;

// Time (ms) after which, if the sensor is not vibrating,
// the LED strip light level should be decreased
const int LED_BOUNCE_MS = 500;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 60;
const uint8_t NEOPIXEL_PIN = 6;
const uint8_t NEOPIXEL_START_PIN = 7;

// LED heartbeat variables
const int BEAT_SEGMENT_LEN = 10;
const int BEAT_PATTERN = 3;
const int BEAT_LONG_MS = 10;
const int BEAT_SHORT_MS = 3;

// Initial LED strip configuration
const int LED_START_DELAY_MS = 150;

// Initialize the NeoPixel instance
Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelStripStart = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_START_PIN, NEO_GRB + NEO_KHZ800);

// NeoPixel active color
uint32_t colorActive = pixelStrip.Color(255, 0, 0);

// Timestamp (millis) of the last state sample
unsigned long lastStateMillis;

// Last observed state
byte lastState;

// Current LED strip light level
int currentStripLevel = 0;

// Timestamp (millis) of the last LED strip light level update
unsigned long lastStripUpdateMillis;

// Activation flag
bool isActivated = false;

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
  int sensorVal = analogRead(SENSOR_PIN);

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

  for (int i = 0; i < sensorBuffer.size(); i++) {
    if (sensorBuffer[i].level > SENSOR_ANALOG_LEVEL_THRESHOLD) {
      return STATE_VIBRATING;
    }
  }

  return STATE_STABLE;
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
void checkCurrentStateAndUpdateLeds() {
  byte currentState = getCurrentState();

  if (currentState == STATE_UNKNOWN ||
      currentState == lastState) {
    return;
  }

  lastState = currentState;

  if (currentState == STATE_VIBRATING) {
    lastStripUpdateMillis = millis();

    if (currentStripLevel < LED_LEVELS) {
      currentStripLevel += 1;
      setPixelsToLevel(currentStripLevel);

      if (currentStripLevel == LED_LEVELS) onMaxLevelReached();
    }
  }

  Serial.print("##### State: ");
  printState(currentState);
  Serial.println();
  Serial.flush();
}

/**
   Start applying the LED strip final pattern.
*/
void onMaxLevelReached() {
  int offset = 0;
  int beatCounter = 0;

  while (true) {
    for (int i = 0; i < NEOPIXEL_NUM; i++) {
      pixelStrip.setPixelColor(i, 0, 0, 0);
    }

    pixelStrip.show();

    if (offset + BEAT_SEGMENT_LEN > NEOPIXEL_NUM) {
      offset = 0;
      beatCounter += 1;
    }

    for (int i = 0; i < BEAT_SEGMENT_LEN; i++) {
      pixelStrip.setPixelColor(i + offset, random(150, 250), 0, 0);
    }

    pixelStrip.show();

    offset += 1;

    int beatRemainder = beatCounter % BEAT_PATTERN;

    if (beatRemainder < (BEAT_PATTERN - 1)) {
      delay(BEAT_SHORT_MS);
    } else if (beatRemainder == (BEAT_PATTERN - 1)) {
      delay(BEAT_LONG_MS);
    }
  }

  currentStripLevel = 0;
  setPixelsToLevel(currentStripLevel);
}

/**
   Turn all the LED strips off.
*/
void deactivateAllLeds() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStripStart.setPixelColor(i, 0, 0, 0);
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStripStart.show();
  pixelStrip.show();
}

/**
   Turn on the LED strip that signals that the program has started.
*/
void activateInitialLeds() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStripStart.setPixelColor(i, colorActive);
    pixelStripStart.show();

    delay(LED_START_DELAY_MS);
  }
}

void setup() {
  Serial.begin(9600);

  isActivated = false;

  pinMode(ACTIVATION_PIN, INPUT_PULLUP);
  pinMode(ACTIVATION_LED_PIN, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(ACTIVATION_LED_PIN, LOW);

  pixelStrip.begin();
  pixelStrip.setBrightness(200);
  pixelStrip.show();

  pixelStripStart.begin();
  pixelStripStart.setBrightness(200);
  pixelStripStart.show();

  lastStateMillis = millis();
  lastStripUpdateMillis = millis();

  deactivateAllLeds();

  Serial.println(">> Starting Reanimathor Phase 02 program");
  Serial.println("Program will be deactivated until the physical switch is turned on");
}

void loop() {
  if (!isActivated && digitalRead(ACTIVATION_PIN) == LOW) {
    Serial.println("## Activating program");
    digitalWrite(ACTIVATION_LED_PIN, HIGH);
    activateInitialLeds();
    isActivated = true;
  }

  if (isActivated) {
    updateSensorBuffer();
    updateStateBufferIfTimePassed();
    checkCurrentStateAndUpdateLeds();
    bouncePixelsIfTimePassed();
  } else {
    deactivateAllLeds();
  }

  delay(LOOP_WAIT_MS);
}
