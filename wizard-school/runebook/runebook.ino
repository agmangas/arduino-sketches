#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>
#include <set>
#include <vector>

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

const byte PATHS_SIZE = 20;
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
    {21, 29, 37, 45},
    {24, 25, 26, 27},
    {24, 30, 36, 42},
    {24, 31, 38, 45},
    {24, 32, 40, 48},
    {27, 33, 39, 45},
    {27, 34, 41, 48},
    {42, 43, 44, 45},
    {45, 46, 47, 48}};

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
    {57, 58, 59, 60, 61, 62, 63}};

/**
   Proximity sensors.
*/

const int PROX_SENSORS_NUM = 9;
const unsigned long PROX_SENSORS_CONFIRMATION_MS = 800;

const int PROX_SENSORS_INDEX[PROX_SENSORS_NUM] = {
    0, 3, 6, 21, 24, 27, 42, 45, 48};

const int PROX_SENSORS_PINS[PROX_SENSORS_NUM] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11};

Atm_button proxSensorsBtn[PROX_SENSORS_NUM];
Atm_controller proxSensorsConfirmControl;

/**
   LED strip (book)
*/

const int LED_BOOK_BRIGHTNESS = 240;
const int LED_BOOK_PIN = 20;
const int LED_BOOK_NUM = 64;
const int LED_BOOK_PATTERN_ANIMATE_MS = 30;
const int LED_BOOK_PATTERN_TAIL_SIZE = 5;
const int LED_BOOK_FADE_MS = 15;
const uint32_t LED_BOOK_COLOR = Adafruit_NeoPixel::Color(128, 0, 128);

Adafruit_NeoPixel ledBook = Adafruit_NeoPixel(LED_BOOK_NUM, LED_BOOK_PIN, NEO_GRB + NEO_KHZ800);

/**
   LED strip (pipes)
*/

const int LED_PIPES_BRIGHTNESS = 240;
const int LED_PIPES_PIN = 22;
const int LED_PIPES_NUM = 300;
const int LED_PIPES_BLOB_SIZE = 2;
const int LED_PIPES_COIL_INI = 65;
const int LED_PIPES_COIL_END = 110;
const int LED_PIPES_COIL_LOOP_INI = 77;
const int LED_PIPES_COIL_LOOP_END = 102;
const int LED_PIPES_ANIMATE_BLOB_DELAY_MS = 15;
const int LED_PIPES_COIL_FILL_DELAY_MS = 100;
const uint32_t LED_PIPES_COLOR = Adafruit_NeoPixel::Color(128, 0, 128);

Adafruit_NeoPixel ledPipes = Adafruit_NeoPixel(LED_PIPES_NUM, LED_PIPES_PIN, NEO_GRB + NEO_KHZ800);

/**
   0: Rabano Vivaz
   1: Aliento Troll
   2: Baba Escarbato
   3: Pluma Fenix
   4: Pelo Canguingo
   5: Bono Loto
   6: Lazo Diablo
   7: Patas Acromantula
   8: Pezuña Thestral
   9: Seta Dose
   10: Culo Kraken
   11: Morros Nutria
*/

const int RUNES_NUM = 12;
const int RUNES_KEY_NUM = 4;

const int RUNES_VALID_KEY[RUNES_KEY_NUM] = {
    0, 6, 4, 8};

const int RUNES_LED_INDEX[RUNES_NUM] = {
    0,
    4,
    8,
    14,
    18,
    22,
    40,
    44,
    48,
    54,
    58,
    62};

std::vector<byte> RUNES_PATHS[RUNES_NUM] = {
    {3, 4, 5, 6,
     3, 10, 17, 24,
     21, 22, 23, 24, 25, 26, 27,
     21, 29, 37, 45,
     27, 33, 39, 45},
    {0, 1, 2, 3, 4, 5, 6,
     6, 13, 20, 27, 34, 41, 48,
     21, 22, 23, 24, 25, 26, 27,
     42, 43, 44, 45, 46, 47, 48,
     21, 28, 35, 42,
     0, 8, 16, 24, 32, 40, 48},
    {3, 9, 15, 21,
     3, 11, 19, 27,
     21, 28, 35, 42,
     27, 34, 41, 48,
     24, 30, 36, 42,
     24, 32, 40, 48},
    {42, 43, 44, 45, 46, 47, 48,
     21, 22, 23, 24, 25, 26, 27,
     0, 7, 14, 21,
     6, 13, 20, 27,
     0, 8, 16, 24, 32, 40, 48,
     6, 12, 18, 24, 30, 36, 42},
    {0, 1, 2, 3, 4, 5, 6,
     42, 43, 44, 45, 46, 47, 48,
     0, 8, 16, 24, 32, 40, 48,
     6, 12, 18, 24, 30, 36, 42},
    {21, 22, 23, 24, 25, 26, 27,
     3, 10, 17, 24, 31, 38, 45,
     0, 8, 16, 24, 32, 40, 48,
     6, 12, 18, 24, 30, 36, 42,
     3, 4, 5, 6,
     42, 43, 44, 45,
     0, 7, 14, 21,
     27, 34, 41, 48},
    {21, 22, 23, 24, 25, 26, 27,
     0, 7, 14, 21,
     6, 13, 20, 27,
     21, 29, 37, 45,
     27, 33, 39, 45,
     6, 12, 18, 24,
     0, 8, 16, 24},
    {0, 1, 2, 3, 4, 5, 6,
     21, 22, 23, 24,
     45, 46, 47, 48,
     0, 7, 14, 21,
     6, 13, 20, 27, 34, 41, 48,
     6, 12, 18, 24,
     24, 31, 38, 45},
    {21, 22, 23, 24, 25, 26, 27,
     3, 10, 17, 24, 31, 38, 45,
     21, 29, 37, 45,
     27, 33, 39, 45},
    {0, 1, 2, 3, 4, 5, 6,
     42, 43, 44, 45, 46, 47, 48,
     6, 12, 18, 24, 30, 36, 42},
    {42, 43, 44, 45, 46, 47, 48,
     3, 11, 19, 27,
     3, 9, 15, 21,
     27, 33, 39, 45,
     21, 29, 37, 45},
    {21, 22, 23, 24, 25, 26, 27,
     3, 10, 17, 24, 31, 38, 45,
     3, 11, 19, 27},
};

/**
  Program state.
*/

const int HISTORY_SENSOR_SIZE = PROX_SENSORS_NUM * 3;
const int HISTORY_PATH_SIZE = HISTORY_SENSOR_SIZE * (PATHS_ITEM_LEN + 1);

int historySensor[HISTORY_SENSOR_SIZE];
int historyPath[HISTORY_PATH_SIZE];
int historyPathLed[HISTORY_PATH_SIZE];
int historyRunes[RUNES_KEY_NUM];

typedef struct programState
{
  int *historySensor;
  int *historyPath;
  int *historyPathLed;
  int *historyRunes;
  unsigned long lastSensorActivation;
} ProgramState;

ProgramState progState = {
    .historySensor = historySensor,
    .historyPath = historyPath,
    .historyPathLed = historyPathLed,
    .historyRunes = historyRunes,
    .lastSensorActivation = 0};

/**
   Runes history functions.
*/

void emptyHistoryRunes()
{
  for (int i = 0; i < RUNES_KEY_NUM; i++)
  {
    progState.historyRunes[i] = -1;
  }
}

bool isHistoryRunesComplete()
{
  return getHistoryPathSize() == RUNES_KEY_NUM;
}

bool isValidRunesCombination()
{
  if (!isHistoryRunesComplete())
  {
    return false;
  }

  for (int i = 0; i < RUNES_KEY_NUM; i++)
  {
    if (progState.historyRunes[i] != RUNES_VALID_KEY[i])
    {
      return false;
    }
  }

  return true;
}

int getHistoryRunesSize()
{
  int size = 0;

  for (int i = 0; i < RUNES_KEY_NUM; i++)
  {
    if (progState.historyRunes[i] == -1)
    {
      break;
    }

    size++;
  }

  return size;
}

void addRuneToHistory(int runeIdx)
{
  if (isHistoryRunesComplete())
  {
    return;
  }

  int nextIdx = -1;

  for (int i = 0; i < RUNES_KEY_NUM; i++)
  {
    if (progState.historyRunes[i] == -1)
    {
      nextIdx = i;
      break;
    }
  }

  if (nextIdx == -1)
  {
    return;
  }

  progState.historyRunes[nextIdx] = runeIdx;
}

/**
   Proximity sensors functions.
*/

bool isSensorPatternConfirmed()
{
  if (progState.lastSensorActivation == 0)
  {
    return false;
  }

  if (progState.historySensor[0] == -1 ||
      progState.historySensor[1] == -1)
  {
    return false;
  }

  unsigned long now = millis();

  if (now < progState.lastSensorActivation)
  {
    Serial.println(F("Millis overflow in sensor pattern confirmation"));
    return true;
  }

  unsigned long diff = now - progState.lastSensorActivation;

  return diff > PROX_SENSORS_CONFIRMATION_MS;
}

void onSensorPatternConfirmed()
{
  Serial.println(F("Sensor pattern confirmed"));

  animateBookLedPattern();

  int runeIdx = getHistoryPathRune();

  if (runeIdx == -1)
  {
    Serial.println("No rune match found");
  }
  else
  {
    Serial.print("History path match on rune: ");
    Serial.println(runeIdx);

    addRuneToHistory(runeIdx);
    fadeBookLedPattern();
    animateRunePipeBlob(runeIdx);
  }

  emptyPathBuffers();
  emptyHistorySensor();
  emptyHistoryPath();
  progState.lastSensorActivation = 0;
}

void onProxSensor(int idx, int v, int up)
{
  Serial.print(F("Proximity sensor activated: "));
  Serial.println(idx);

  progState.lastSensorActivation = millis();

  addHistorySensor(idx);
  refreshHistoryPath();
}

void initProximitySensors()
{
  for (int i = 0; i < PROX_SENSORS_NUM; i++)
  {
    proxSensorsBtn[i]
        .begin(PROX_SENSORS_PINS[i])
        .onPress(onProxSensor, i);
  }

  proxSensorsConfirmControl
      .begin()
      .IF(isSensorPatternConfirmed)
      .onChange(true, onSensorPatternConfirmed);
}

/**
  Function to reverse order of items in array.
  Attribution to:
  https://stackoverflow.com/a/22978241
*/

void reverseRange(int *arr, int lft, int rgt)
{
  while (lft < rgt)
  {
    int temp = arr[lft];
    arr[lft++] = arr[rgt];
    arr[rgt--] = temp;
  }
}

/**
   Functions to handle path history.
*/

int getHistoryPathRune()
{
  int currSize = getHistoryPathSize();

  if (currSize == 0)
  {
    return -1;
  }

  std::set<int> histPathSet(
      progState.historyPath,
      progState.historyPath + currSize);

  for (int i = 0; i < RUNES_NUM; i++)
  {
    std::set<int> runeSet(
        RUNES_PATHS[i].begin(),
        RUNES_PATHS[i].end());

    if (histPathSet == runeSet)
    {
      return i;
    }
  }

  return -1;
}

int getHistoryPathSize()
{
  int currSize = 0;

  for (int i = 0; i < HISTORY_PATH_SIZE; i++)
  {
    if (progState.historyPath[i] != -1)
    {
      currSize++;
    }
    else
    {
      break;
    }
  }

  return currSize;
}

void emptyHistorySensor()
{
  Serial.println(F("Emptying sensor history"));

  for (int i = 0; i < HISTORY_SENSOR_SIZE; i++)
  {
    progState.historySensor[i] = -1;
  }
}

void emptyHistoryPath()
{
  Serial.println(F("Emptying path history"));

  for (int i = 0; i < HISTORY_PATH_SIZE; i++)
  {
    progState.historyPath[i] = -1;
    progState.historyPathLed[i] = -1;
  }
}

bool isSensorAdjacent(int idxOne, int idxOther)
{
  Serial.print("Checking adjacency: ");
  Serial.print(idxOne);
  Serial.print(" - ");
  Serial.println(idxOther);

  bool isMatchAsc;
  bool isMatchDesc;

  for (int i = 0; i < PATHS_SIZE; i++)
  {
    isMatchAsc = PATHS[i][0] == idxOne &&
                 PATHS[i][PATHS_ITEM_LEN - 1] == idxOther;

    isMatchDesc = PATHS[i][0] == idxOther &&
                  PATHS[i][PATHS_ITEM_LEN - 1] == idxOne;

    if (isMatchAsc || isMatchDesc)
    {
      return true;
    }
  }

  return false;
}

void addHistorySensor(int sensorIdx)
{
  if (sensorIdx < 0 || sensorIdx >= PROX_SENSORS_NUM)
  {
    Serial.println(F("Sensor index is out of range"));
    return;
  }

  int sensorMatrixIdx = PROX_SENSORS_INDEX[sensorIdx];
  int nextIdx = -1;

  for (int i = 0; i < HISTORY_SENSOR_SIZE; i++)
  {
    if (progState.historySensor[i] == -1)
    {
      nextIdx = i;
      break;
    }
  }

  if (nextIdx == -1)
  {
    Serial.println(F("Full sensor history"));
    return;
  }

  if (nextIdx > 0)
  {
    int prevSensorMatrixIdx = progState.historySensor[nextIdx - 1];

    if (!isSensorAdjacent(prevSensorMatrixIdx, sensorMatrixIdx))
    {
      Serial.print(F("Sensor "));
      Serial.print(sensorMatrixIdx);
      Serial.print(F(" is not adjacent to the previous: "));
      Serial.println(prevSensorMatrixIdx);

      return;
    }
  }

  Serial.print(F("Adding sensor to history: "));
  Serial.println(sensorMatrixIdx);

  progState.historySensor[nextIdx] = sensorMatrixIdx;
}

void refreshHistoryPath()
{
  emptyHistoryPath();

  int start;
  int finish;
  int pathPivot = 0;

  for (int i = 1; i < HISTORY_SENSOR_SIZE; i++)
  {
    if (progState.historySensor[i] == -1)
    {
      Serial.println(F("No more items in sensor history"));
      return;
    }

    start = progState.historySensor[i - 1];
    finish = progState.historySensor[i];

    updatePathBuffers(start, finish);

    if (isEmptyPathBuffers())
    {
      Serial.println(F("Unexpected empty path buffers"));
      return;
    }

    for (int j = 0; j < PATHS_ITEM_LEN; j++)
    {
      progState.historyPath[pathPivot] = pathBuf[j];
      progState.historyPathLed[pathPivot] = ledPathBuf[j];

      pathPivot++;
    }
  }
}

/**
  Shortest path and path buffer functions.
*/

void emptyPathBuffers()
{
  for (int i = 0; i < PATHS_ITEM_LEN; i++)
  {
    pathBuf[i] = -1;
    ledPathBuf[i] = -1;
  }
}

bool isEmptyPathBuffers()
{
  for (int i = 0; i < PATHS_ITEM_LEN; i++)
  {
    if (pathBuf[i] == -1 || ledPathBuf[i] == -1)
    {
      return true;
    }
  }

  return false;
}

void updatePathBuffers(int init, int finish)
{
  emptyPathBuffers();

  int matchIdx = -1;

  bool isMatchAsc;
  bool isMatchDesc;

  for (int i = 0; i < PATHS_SIZE; i++)
  {
    isMatchAsc = PATHS[i][0] == init &&
                 PATHS[i][PATHS_ITEM_LEN - 1] == finish;

    isMatchDesc = PATHS[i][0] == finish &&
                  PATHS[i][PATHS_ITEM_LEN - 1] == init;

    if (isMatchAsc || isMatchDesc)
    {
      matchIdx = i;
      break;
    }
  }

  if (matchIdx == -1)
  {
    Serial.println(F("No shortest path could be found"));
    return;
  }

  for (int i = 0; i < PATHS_ITEM_LEN; i++)
  {
    pathBuf[i] = PATHS[matchIdx][i];
  }

  if (isMatchDesc)
  {
    reverseRange(pathBuf, 0, PATHS_ITEM_LEN - 1);
  }

  int idxCol;
  int idxRow;

  for (int i = 0; i < PATHS_ITEM_LEN; i++)
  {
    idxCol = pathBuf[i] % MATRIX_SIZE;
    idxRow = floor(((float)pathBuf[i]) / MATRIX_SIZE);
    ledPathBuf[i] = LED_MAP[idxRow][idxCol];
  }
}

/**
   LED functions.
*/

void initLeds()
{
  ledBook.begin();
  ledBook.setBrightness(LED_BOOK_BRIGHTNESS);
  ledBook.show();

  clearLedsBook();

  ledPipes.begin();
  ledPipes.setBrightness(LED_PIPES_BRIGHTNESS);
  ledPipes.show();

  clearLedsPipes();
}

void clearLedsBook()
{
  ledBook.clear();
  ledBook.show();
}

void clearLedsPipes()
{
  ledPipes.clear();
  ledPipes.show();
}

void fadeBookLedPattern()
{
  clearLedsBook();

  const int iniVal = 0;
  const int endVal = 250;

  for (int k = iniVal; k < endVal; k++)
  {
    for (int i = 0; i < HISTORY_PATH_SIZE; i++)
    {
      if (progState.historyPathLed[i] == -1)
      {
        break;
      }

      ledBook.setPixelColor(progState.historyPathLed[i], 0, 0, k);
    }

    ledBook.show();

    delay(LED_BOOK_FADE_MS);
  }
}

void animateBookLedPattern()
{
  int pivotIdx;

  for (int i = 0; i < HISTORY_PATH_SIZE; i++)
  {
    clearLedsBook();

    if (progState.historyPathLed[i] == -1)
    {
      break;
    }

    for (int j = 0; j < LED_BOOK_PATTERN_TAIL_SIZE; j++)
    {
      pivotIdx = i + j;

      if (progState.historyPathLed[pivotIdx] == -1)
      {
        break;
      }

      ledBook.setPixelColor(progState.historyPathLed[pivotIdx], LED_BOOK_COLOR);
    }

    ledBook.show();

    delay(LED_BOOK_PATTERN_ANIMATE_MS);
  }

  clearLedsBook();
}

int getLedCoilLoopSize(int runesHistoryLen)
{
  float coilProgress = (float)runesHistoryLen / RUNES_KEY_NUM;
  return (LED_PIPES_COIL_END - LED_PIPES_COIL_INI) * coilProgress;
}

int getLedCoilLoopSize()
{
  return getLedCoilLoopSize(getHistoryRunesSize());
}

uint32_t getPipeColor()
{
  return Adafruit_NeoPixel::Color(random(150, 250), 0, random(150, 250));
}

void animateRunePipeBlob(int runeIdx)
{
  int prevRunesLen = getHistoryRunesSize();
  prevRunesLen = prevRunesLen > 0 ? prevRunesLen - 1 : 0;
  int prevCoilSize = getLedCoilLoopSize(prevRunesLen);

  int blobEnd = LED_PIPES_COIL_END - prevCoilSize;
  int pivotIdx = RUNES_LED_INDEX[runeIdx];

  while (pivotIdx < blobEnd)
  {
    for (int i = 0; i < blobEnd; i++)
    {
      ledPipes.setPixelColor(i, 0);
    }

    for (int i = 0; i < LED_PIPES_BLOB_SIZE; i++)
    {
      ledPipes.setPixelColor(pivotIdx + i, getPipeColor());
    }

    pivotIdx++;
    ledPipes.show();
    delay(LED_PIPES_ANIMATE_BLOB_DELAY_MS);
  }

  for (int i = 0; i < blobEnd; i++)
  {
    ledPipes.setPixelColor(i, 0);
  }

  ledPipes.show();

  int coilIni = LED_PIPES_COIL_END - getLedCoilLoopSize();

  for (int i = blobEnd; i >= coilIni; i--)
  {
    ledPipes.setPixelColor(i, getPipeColor());
    ledPipes.show();
    delay(LED_PIPES_COIL_FILL_DELAY_MS);
  }

  clearLedsPipes();
}

void refreshLedsBook()
{
  ledBook.clear();

  for (int i = 0; i < HISTORY_PATH_SIZE; i++)
  {
    if (progState.historyPathLed[i] == -1)
    {
      break;
    }

    ledBook.setPixelColor(progState.historyPathLed[i], LED_BOOK_COLOR);
  }

  ledBook.show();
}

void refreshLedsPipes()
{
  ledPipes.clear();

  int currRuneIdx;

  for (int i = 0; i < RUNES_KEY_NUM; i++)
  {
    if (progState.historyRunes[i] == -1)
    {
      break;
    }

    currRuneIdx = RUNES_LED_INDEX[progState.historyRunes[i]];

    for (int j = 0; j < LED_PIPES_BLOB_SIZE; j++)
    {
      ledPipes.setPixelColor(currRuneIdx + j, getPipeColor());
    }
  }

  int coilSize = getLedCoilLoopSize();

  for (int i = 0; i < coilSize; i++)
  {
    ledPipes.setPixelColor(LED_PIPES_COIL_END - i, getPipeColor());
  }

  ledPipes.show();
}

/**
   Entrypoint.
*/

void setup()
{
  Serial.begin(9600);

  emptyHistoryRunes();
  emptyPathBuffers();
  emptyHistorySensor();
  emptyHistoryPath();
  initProximitySensors();
  initLeds();

  Serial.println(F(">> Starting Runebook program"));
}

void loop()
{
  automaton.run();
  refreshLedsBook();
  refreshLedsPipes();
}
