#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
  Shortest paths in the LED matrix.
  [
    {00, 01, 02, 03, 04, 05, 06},
    {07, 08, 09, 10, 11, 12, 13},
    {14, 15, 16, 17, 18, 19, 20},
    {21, 22, 23, 24, 25, 26, 27},
    {28, 29, 30, 31, 32, 33, 34},
    {35, 36, 37, 38, 39, 40, 41},
    {42, 43, 44, 45, 46, 47, 48}
  ]
*/

const byte PATHS_SIZE = 18;
const byte PATHS_ITEM_LEN = 4;

int pathBuf[PATHS_ITEM_LEN];
int ledPathBuf[PATHS_ITEM_LEN];

const byte PATHS[PATHS_SIZE][PATHS_ITEM_LEN] = {
  {0, 1, 2, 3},
  {0, 7, 14, 21},
  {0, 8, 16, 24},
  {3, 4, 5, 6},
  {3, 9, 15, 21},
  {3, 10, 17, 24},
  {3, 11, 19, 27},
  {6, 12, 18, 24},
  {6, 13, 20, 27},
  {21, 22, 23, 24},
  {21, 28, 35, 42},
  {24, 25, 26, 27},
  {24, 30, 36, 42},
  {24, 31, 38, 45},
  {24, 32, 40, 48},
  {27, 33, 39, 45},
  {27, 34, 41, 48},
  {42, 43, 44, 45}
};

/**
   LED index map.
*/

const byte MATRIX_SIZE = 7;

const byte LED_MAP[MATRIX_SIZE][MATRIX_SIZE] = {
  {0, 1, 2, 3, 4, 5, 6},
  {15, 14, 13, 12, 11, 10, 9},
  {19, 20, 21, 22, 23, 24, 25},
  {34, 33, 32, 31, 30, 29, 28},
  {37, 38, 39, 40, 41, 42, 43},
  {53, 52, 51, 50, 49, 48, 47},
  {57, 58, 59, 60, 61, 62, 63}
};

/**
   Proximity sensors.
*/

const int PROX_SENSORS_NUM = 9;
const int PROX_SENSORS_SAMPLERATE = 50;
const int PROX_SENSORS_RANGE_MIN = 0;
const int PROX_SENSORS_RANGE_MAX = 1000;
const int PROX_SENSORS_THRESHOLD = 930;

const int PROX_SENSORS_INDEX[PROX_SENSORS_NUM] = {
  0, 3, 6, 21, 24, 27, 42, 45, 48
};

const int PROX_SENSORS_PINS[PROX_SENSORS_NUM] = {
  A0, A1, A2, A3, A4, A5, A6, A7, A8
};

Atm_analog proxSensorsAnalog[PROX_SENSORS_NUM];
Atm_controller proxSensorsControl[PROX_SENSORS_NUM];

/**
   LED strip.
*/

const int LED_BOOK_BRIGHTNESS = 150;
const int LED_BOOK_PIN = 10;
const int LED_BOOK_NUM = 64;
const uint32_t LED_BOOK_COLOR = Adafruit_NeoPixel::Color(255, 105, 97);

Adafruit_NeoPixel ledBook = Adafruit_NeoPixel(LED_BOOK_NUM, LED_BOOK_PIN, NEO_GRB + NEO_KHZ800);

/**
  Program state.
*/

const int HISTORY_SENSOR_SIZE = PROX_SENSORS_NUM * 3;
const int HISTORY_PATH_SIZE = HISTORY_SENSOR_SIZE * (PATHS_ITEM_LEN + 1);

int historySensor[HISTORY_SENSOR_SIZE];
int historyPath[HISTORY_PATH_SIZE];
int historyPathLed[HISTORY_PATH_SIZE];

typedef struct programState {
  int* historySensor;
  int* historyPath;
  int* historyPathLed;
} ProgramState;

ProgramState progState = {
  .historySensor = historySensor,
  .historyPath = historyPath,
  .historyPathLed = historyPathLed
};

/**
   Proximity sensors functions.
*/

void onProxSensor(int idx, int v, int up) {
  Serial.print(F("Proximity sensor activated: "));
  Serial.println(idx);

  addHistorySensor(idx);
  refreshHistoryPath();
}

void initProximitySensors() {
  for (int i = 0; i < PROX_SENSORS_NUM; i++) {
    proxSensorsAnalog[i]
    .begin(PROX_SENSORS_PINS[i], PROX_SENSORS_SAMPLERATE)
    .range(PROX_SENSORS_RANGE_MIN, PROX_SENSORS_RANGE_MAX);

    proxSensorsControl[i]
    .begin()
    .IF(proxSensorsAnalog[i], '<', PROX_SENSORS_THRESHOLD)
    .onChange(true, onProxSensor, i);
  }
}

/**
  Function to reverse order of items in array.
  Attribution to:
  https://stackoverflow.com/a/22978241
*/

void reverseRange(int* arr, int lft, int rgt) {
  while (lft < rgt) {
    int temp = arr[lft];
    arr[lft++] = arr[rgt];
    arr[rgt--] = temp;
  }
}

/**
   Functions to handle path history.
*/

void emptyHistorySensor() {
  Serial.println(F("Emptying sensor history"));

  for (int i = 0; i < HISTORY_SENSOR_SIZE; i++) {
    progState.historySensor[i] = -1;
  }
}

void emptyHistoryPath() {
  Serial.println(F("Emptying path history"));

  for (int i = 0; i < HISTORY_PATH_SIZE; i++) {
    progState.historyPath[i] = -1;
    progState.historyPathLed[i] = -1;
  }
}

bool isSensorAdjacent(int idxOne, int idxOther) {
  bool isMatchAsc;
  bool isMatchDesc;

  for (int i = 0; i < PATHS_SIZE; i++) {
    isMatchAsc = PATHS[i][0] == idxOne &&
                 PATHS[i][PATHS_ITEM_LEN - 1] == idxOther;

    isMatchDesc = PATHS[i][0] == idxOther &&
                  PATHS[i][PATHS_ITEM_LEN - 1] == idxOne;

    if (isMatchAsc || isMatchDesc) {
      return true;
    }
  }

  return false;
}

void addHistorySensor(int sensorIdx) {
  if (sensorIdx < 0 || sensorIdx >= PROX_SENSORS_NUM) {
    Serial.println(F("Sensor index is out of range"));
    return;
  }

  int nextIdx = -1;

  for (int i = 0 ; i < HISTORY_SENSOR_SIZE; i++) {
    if (progState.historySensor[i] == -1) {
      nextIdx = i;
      break;
    }
  }

  if (nextIdx == -1) {
    Serial.println(F("Full sensor history"));
    return;
  }

  if (nextIdx > 0 &&
      !isSensorAdjacent(progState.historySensor[nextIdx - 1], sensorIdx)) {
    Serial.print(F("Sensor "));
    Serial.print(sensorIdx);
    Serial.print(F(" is not adjacent to the previous: "));
    Serial.println(progState.historySensor[nextIdx - 1]);
    return;
  }

  Serial.print(F("Adding sensor to history: "));
  Serial.println(sensorIdx);

  progState.historySensor[nextIdx] = sensorIdx;
}

void refreshHistoryPath() {
  emptyHistoryPath();

  int init;
  int finish;
  int pathPivot = 0;

  for (int i = 1; i < HISTORY_SENSOR_SIZE; i++) {
    if (progState.historySensor[i] == -1) {
      Serial.println(F("No more items in sensor history"));
      return;
    }

    init = progState.historySensor[i - 1];
    finish = progState.historySensor[i];

    updatePathBuffers(init, finish);

    if (isEmptyPathBuffers()) {
      Serial.println(F("Unexpected empty path buffers"));
      return;
    }

    for (int j = 0; j < PATHS_ITEM_LEN; j++) {
      Serial.print(F("Adding item to path history: "));
      Serial.println(pathBuf[j]);

      Serial.print(F("Adding item to LED path history: "));
      Serial.println(ledPathBuf[j]);

      progState.historyPath[pathPivot] = pathBuf[j];
      progState.historyPathLed[pathPivot] = ledPathBuf[j];

      pathPivot++;
    }
  }
}

/**
  Functions to update the path buffer with the shortest path.
*/

void emptyPathBuffers() {
  Serial.println(F("Emptying path buffers"));

  for (int i = 0; i < PATHS_ITEM_LEN; i++) {
    pathBuf[i] = -1;
    ledPathBuf[i] = -1;
  }
}

bool isEmptyPathBuffers() {
  for (int i = 0; i < PATHS_ITEM_LEN; i++) {
    if (pathBuf[i] == -1 || ledPathBuf[i] == -1) {
      return true;
    }
  }

  return false;
}

void updatePathBuffers(int init, int finish) {
  Serial.print(F("Getting shortest path from "));
  Serial.print(init);
  Serial.print(F(" to "));
  Serial.println(finish);

  emptyPathBuffers();

  int matchIdx = -1;

  bool isMatchAsc;
  bool isMatchDesc;

  for (int i = 0; i < PATHS_SIZE; i++) {
    isMatchAsc = PATHS[i][0] == init &&
                 PATHS[i][PATHS_ITEM_LEN - 1] == finish;

    isMatchDesc = PATHS[i][0] == finish &&
                  PATHS[i][PATHS_ITEM_LEN - 1] == init;

    if (isMatchAsc || isMatchDesc) {
      matchIdx = i;
      break;
    }
  }

  if (matchIdx == -1) {
    Serial.println(F("No shortest path could be found"));
    return;
  }

  for (int i = 0; i < PATHS_ITEM_LEN; i++) {
    pathBuf[i] = PATHS[matchIdx][i];
  }

  if (isMatchDesc) {
    reverseRange(pathBuf, 0, PATHS_ITEM_LEN - 1);
  }

  int idxCol;
  int idxRow;

  for (int i = 0; i < PATHS_ITEM_LEN; i++) {
    idxCol = pathBuf[i] % MATRIX_SIZE;
    idxRow = floor(((float) pathBuf[i]) / MATRIX_SIZE);
    ledPathBuf[i] = LED_MAP[idxRow][idxCol];
  }
}

/**
   LED functions.
*/

void initLeds() {
  ledBook.begin();
  ledBook.setBrightness(LED_BOOK_BRIGHTNESS);
  ledBook.show();

  clearLedsBook();
}

void clearLedsBook() {
  ledBook.clear();
  ledBook.show();
}

void refreshLedsBook() {
  clearLedsBook();

  for (int i = 0; i < HISTORY_PATH_SIZE; i++) {
    if (progState.historyPathLed[i] == -1) {
      break;
    }

    ledBook.setPixelColor(progState.historyPathLed[i], LED_BOOK_COLOR);
  }

  ledBook.show();
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  emptyPathBuffers();
  emptyHistorySensor();
  emptyHistoryPath();
  initProximitySensors();
  initLeds();

  Serial.println(F(">> Starting Runebook program"));
}

void loop() {
  automaton.run();
  refreshLedsBook();
}
