#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>
#include <set>
#include <vector>
#include <Atm_servo.h>
#include "rdm630.h"

/**
 * RFID reader.
 */

const byte PIN_RFID_RX = 26;
const byte PIN_RFID_TX = 27;

RDM6300 rfidReader(PIN_RFID_RX, PIN_RFID_TX);

const byte NUM_VALID_TAGS = 2;

String validTags[NUM_VALID_TAGS] = {
    "1D0028450000",
    "1D0027A3EA00"};

/**
 * Relays.
 */

const int RELAY_PIN_RFID = 24;
const int RELAY_PIN_MICS = 52;

/**
 * Shortest paths in the LED matrix.
 *  [
 *    {00, 01, 02, 03, 04, 05, 06},
 *    {07, 08, 09, 10, 11, 12, 13},
 *    {14, 15, 16, 17, 18, 19, 20},
 *    {21, 22, 23, 24, 25, 26, 27},
 *    {28, 29, 30, 31, 32, 33, 34},
 *    {35, 36, 37, 38, 39, 40, 41},
 *    {42, 43, 44, 45, 46, 47, 48}
 *  ]
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
 * LED index map.
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
 * Proximity sensors.
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
 * Microphones.
 */

const int MICS_NUM = 2;
const int MICS_SAMPLE_RATE_MS = 50;
const int MICS_RANGE_MIN = 0;
const int MICS_RANGE_MAX = 10;
const int MICS_VALID_COUNTER_TARGET = 15;

// Inclusive
const int MICS_THRESHOLD_MIN = 3;
const int MICS_THRESHOLD_MAX = MICS_RANGE_MAX;

const byte MICS_PIN[MICS_NUM] = {
    A5, A6};

Atm_analog mics[MICS_NUM];

const int MICS_TIMER_MS = 1000;
const int MICS_RECENT_TIMER_RATIO = 1;

/**
 * LED strip (microphones).
 */

const int LED_MICS_BRIGHTNESS = 200;
const int LED_MICS_PINS[MICS_NUM] = {28, 30};
const int LED_MICS_NUM[MICS_NUM] = {10, 10};
const uint32_t LED_MICS_COLOR = Adafruit_NeoPixel::Color(255, 131, 0);
const int LED_MICS_HIDE_LT_LEVEL = 2;

Adafruit_NeoPixel ledMic01 = Adafruit_NeoPixel(
    LED_MICS_NUM[0],
    LED_MICS_PINS[0],
    NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel ledMic02 = Adafruit_NeoPixel(
    LED_MICS_NUM[1],
    LED_MICS_PINS[1],
    NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel ledMics[MICS_NUM] = {
    ledMic01,
    ledMic02};

Atm_timer micsTimer;

/**
 * LED strip (book).
 */

const int LED_BOOK_BRIGHTNESS = 240;
const int LED_BOOK_PIN = 20;
const int LED_BOOK_NUM = 64;
const int LED_BOOK_PATTERN_ANIMATE_MS = 30;
const int LED_BOOK_PATTERN_TAIL_SIZE = 5;
const int LED_BOOK_FADE_MS = 15;
const uint32_t LED_BOOK_COLOR = Adafruit_NeoPixel::Color(128, 0, 128);

Adafruit_NeoPixel ledBook = Adafruit_NeoPixel(
    LED_BOOK_NUM,
    LED_BOOK_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * LED strip (pipes).
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
const int LED_PIPES_BLOB_NUM_PULSES = 12;
const int LED_PIPES_BLOB_PULSE_DELAY = 50;
const int LED_PIPES_SUCCESS_ANIMATE_DELAY = 20;
const int LED_PIPES_ERROR_DELAY_STEP = 5;
const int LED_PIPES_ERROR_NUM_ITERS = 20;
const int LED_PIPES_ERROR_INI_DELAY = 10;
const uint32_t LED_PIPES_COLOR = Adafruit_NeoPixel::Color(128, 0, 128);

Adafruit_NeoPixel ledPipes = Adafruit_NeoPixel(
    LED_PIPES_NUM,
    LED_PIPES_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * 0: Rabano Vivaz
 * 1: Aliento Troll
 * 2: Baba Escarbato
 * 3: Pluma Fenix
 * 4: Pelo Canguingo
 * 5: Bono Loto
 * 6: Lazo Diablo
 * 7: Patas Acromantula
 * 8: Pezuña Thestral
 * 9: Seta Dose
 * 10: Culo Kraken
 * 11: Morros Nutria
 * 12: RESET
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

std::vector<byte> RUNE_RESET_PATH = {
    0, 1, 2, 3, 4, 5, 6,
    42, 43, 44, 45, 46, 47, 48,
    0, 7, 14, 21, 28, 35, 42,
    6, 13, 20, 27, 34, 41, 48};

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
     3, 11, 19, 27}};

/**
 * Servo.
 */

const int SERVO_PIN = 12;
const int SERVO_STEP_SIZE = 180;
const int SERVO_STEP_TIME = 0;
const int SERVO_TIMER_MS = 100;
const int SERVO_POS_HI = 20;
const int SERVO_POS_LO = 150;
const int SERVO_REPEATS = 100;

Atm_servo servo;
Atm_timer timerServo;

/**
 * Program state.
 */

const int HISTORY_SENSOR_SIZE = PROX_SENSORS_NUM * 3;
const int HISTORY_PATH_SIZE = HISTORY_SENSOR_SIZE * (PATHS_ITEM_LEN + 1);

int historySensor[HISTORY_SENSOR_SIZE];
int historyPath[HISTORY_PATH_SIZE];
int historyPathLed[HISTORY_PATH_SIZE];
int historyRunes[RUNES_KEY_NUM];
int micsLedLevel[MICS_NUM];
unsigned long micsLastRead[MICS_NUM];

typedef struct programState
{
    bool isRunePhaseComplete;
    bool isRfidPhaseComplete;
    bool isMicsPhaseComplete;
    int *historySensor;
    int *historyPath;
    int *historyPathLed;
    int *historyRunes;
    unsigned long lastSensorActivation;
    int *micsLedLevel;
    unsigned long *micsLastRead;
    int micsValidLevelCounter;
} ProgramState;

ProgramState progState = {
    .isRunePhaseComplete = false,
    .isRfidPhaseComplete = false,
    .isMicsPhaseComplete = false,
    .historySensor = historySensor,
    .historyPath = historyPath,
    .historyPathLed = historyPathLed,
    .historyRunes = historyRunes,
    .lastSensorActivation = 0,
    .micsLedLevel = micsLedLevel,
    .micsLastRead = micsLastRead,
    .micsValidLevelCounter = 0};

bool shouldListenToProxSensors()
{
    return progState.isRunePhaseComplete == false;
}

bool shouldListenToRfid()
{
    return progState.isRunePhaseComplete == true &&
           progState.isRfidPhaseComplete == false;
}

bool shouldListenToMics()
{
    return progState.isRunePhaseComplete == true &&
           progState.isRfidPhaseComplete == true &&
           progState.isMicsPhaseComplete == false;
}

/**
 * Servo functions.
 */

void onServoTimer(int idx, int v, int up)
{
    int servoPos = up % 2 == 0 ? SERVO_POS_HI : SERVO_POS_LO;
    Serial.print(F("Moving servo: "));
    Serial.println(servoPos);
    servo.position(servoPos);
}

void initServo()
{
    servo
        .begin(SERVO_PIN)
        .step(SERVO_STEP_SIZE, SERVO_STEP_TIME);

    timerServo.begin(SERVO_TIMER_MS)
        .repeat(SERVO_REPEATS)
        .onTimer(onServoTimer);
}

/**
 * Runes history functions.
 */

bool isRuneInHistory(int runeIdx)
{
    for (int i = 0; i < RUNES_KEY_NUM; i++)
    {
        if (progState.historyRunes[i] == runeIdx)
        {
            return true;
        }
    }

    return false;
}

void emptyHistoryRunes()
{
    for (int i = 0; i < RUNES_KEY_NUM; i++)
    {
        progState.historyRunes[i] = -1;
    }
}

bool isHistoryRunesComplete()
{
    return getHistoryRunesSize() == RUNES_KEY_NUM;
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
 * Proximity sensors functions.
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

void cleanSensorState()
{
    emptyPathBuffers();
    emptyHistorySensor();
    emptyHistoryPath();
    progState.lastSensorActivation = 0;
}

void onRunePhaseComplete()
{
    Serial.println("Rune phase complete");
    animateLedPipesSuccess();
    Serial.println("Starting servo timer");
    timerServo.start();
    progState.isRunePhaseComplete = true;
    clearLedsBook();
    ledPipes.fill(getPipeColor());
    ledPipes.show();
}

void onSensorPatternConfirmed()
{
    Serial.println(F("Pattern confirmed"));

    animateBookLedPattern();

    int runeIdx = getHistoryPathRune();

    if (isHistoryPathResetRune())
    {
        Serial.println("Reset rune");
        cleanSensorState();
        emptyHistoryRunes();
    }
    else if (runeIdx == -1)
    {
        Serial.println("No rune match found");
    }
    else
    {
        Serial.print("Match on rune: ");
        Serial.println(runeIdx);

        if (!isHistoryRunesComplete() && !isRuneInHistory(runeIdx))
        {
            addRuneToHistory(runeIdx);
            fadeBookLedPattern();
            animateRunePipeBlob(runeIdx);
        }
    }

    cleanSensorState();

    bool runesComplete = isHistoryRunesComplete();
    bool validRunes = isValidRunesCombination();

    if (runesComplete && validRunes && !progState.isRunePhaseComplete)
    {
        onRunePhaseComplete();
    }
    else if (runesComplete && !validRunes)
    {
        Serial.println("Invalid runes combination");
        emptyHistoryRunes();
        animateLedPipesError();
    }
}

void onProxSensor(int idx, int v, int up)
{
    if (!shouldListenToProxSensors())
    {
        return;
    }

    Serial.print(F("Sensor activated: "));
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
 * Function to reverse order of items in array.
 * Attribution to:
 * https://stackoverflow.com/a/22978241
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
 * Functions to handle path history.
 */

bool isHistoryPathResetRune()
{
    std::set<int> histPathSet(
        progState.historyPath,
        progState.historyPath + getHistoryPathSize());

    std::set<int> resetRuneSet(
        RUNE_RESET_PATH.begin(),
        RUNE_RESET_PATH.end());

    return histPathSet == resetRuneSet;
}

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
    for (int i = 0; i < HISTORY_SENSOR_SIZE; i++)
    {
        progState.historySensor[i] = -1;
    }
}

void emptyHistoryPath()
{
    for (int i = 0; i < HISTORY_PATH_SIZE; i++)
    {
        progState.historyPath[i] = -1;
        progState.historyPathLed[i] = -1;
    }
}

bool isSensorAdjacent(int idxOne, int idxOther)
{
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
            Serial.print(F(" not adjacent to: "));
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
 * Shortest path and path buffer functions.
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
 * LED functions.
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

    for (int i = 0; i < MICS_NUM; i++)
    {
        ledMics[i].begin();
        ledMics[i].setBrightness(LED_MICS_BRIGHTNESS);
        ledMics[i].clear();
        ledMics[i].show();
    }
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
    return Adafruit_NeoPixel::Color(0, random(150, 250), random(150, 250));
}

void animateRunePipeBlob(int runeIdx)
{
    int prevRunesLen = getHistoryRunesSize();
    prevRunesLen = prevRunesLen > 0 ? prevRunesLen - 1 : 0;
    int prevCoilSize = getLedCoilLoopSize(prevRunesLen);

    int blobEnd = LED_PIPES_COIL_END - prevCoilSize;
    int pivotIdx = RUNES_LED_INDEX[runeIdx];

    for (int i = 0; i < blobEnd; i++)
    {
        ledPipes.setPixelColor(i, 0);
    }

    ledPipes.show();

    for (int k = 0; k < LED_PIPES_BLOB_NUM_PULSES; k++)
    {
        for (int i = 0; i < LED_PIPES_BLOB_SIZE; i++)
        {
            ledPipes.setPixelColor(pivotIdx + i, getPipeColor());
        }

        ledPipes.show();
        delay(LED_PIPES_BLOB_PULSE_DELAY);

        for (int i = 0; i < LED_PIPES_BLOB_SIZE; i++)
        {
            ledPipes.setPixelColor(pivotIdx + i, 0);
        }

        ledPipes.show();
        delay(LED_PIPES_BLOB_PULSE_DELAY);
    }

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
}

void animateLedPipesSuccess()
{
    clearLedsPipes();

    int incPivotIdx = LED_PIPES_COIL_END;
    int decPivotIdx = LED_PIPES_COIL_END;

    while (incPivotIdx < LED_PIPES_NUM || decPivotIdx >= 0)
    {
        if (incPivotIdx < LED_PIPES_NUM)
        {
            ledPipes.setPixelColor(incPivotIdx, getPipeColor());
            incPivotIdx++;
        }

        if (decPivotIdx >= 0)
        {
            ledPipes.setPixelColor(decPivotIdx, getPipeColor());
            decPivotIdx--;
        }

        ledPipes.show();
        delay(LED_PIPES_SUCCESS_ANIMATE_DELAY);
    }
}

void animateLedPipesError()
{
    clearLedsPipes();

    int delayMs = LED_PIPES_ERROR_INI_DELAY;

    for (int k = 0; k < LED_PIPES_ERROR_NUM_ITERS; k++)
    {
        for (int i = 0; i < LED_PIPES_NUM; i++)
        {
            ledPipes.setPixelColor(i, random(150, 250), 0, 0);
        }

        ledPipes.show();
        delay(delayMs);

        for (int i = 0; i < LED_PIPES_NUM; i++)
        {
            ledPipes.setPixelColor(i, 0);
        }

        ledPipes.show();
        delay(delayMs);

        delayMs = delayMs + LED_PIPES_ERROR_DELAY_STEP;
    }
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

void refreshLedsMics()
{
    for (int i = 0; i < MICS_NUM; i++)
    {
        ledMics[i].clear();

        if (progState.micsLedLevel[i] > LED_MICS_HIDE_LT_LEVEL)
        {
            for (int j = 0; j < progState.micsLedLevel[i]; j++)
            {
                ledMics[i].setPixelColor(j, LED_MICS_COLOR);
            }
        }

        ledMics[i].show();
    }
}

/**
 * Microphones functions.
 */

void onMicChange(int idx, int v, int up)
{
    if (!shouldListenToMics())
    {
        return;
    }

    Serial.print(F("Mics #"));
    Serial.print(idx);
    Serial.print(F(" :: "));
    Serial.println(v);

    if (v > progState.micsLedLevel[idx])
    {
        progState.micsLedLevel[idx] = v;
        refreshLedsMics();

        Serial.print(F("Mics :: #"));
        Serial.print(idx);
        Serial.print(F(" :: +LED :: "));
        Serial.println(progState.micsLedLevel[idx]);
    }

    progState.micsLastRead[idx] = millis();
}

void initMics()
{
    for (int i = 0; i < MICS_NUM; i++)
    {
        mics[i]
            .begin(MICS_PIN[i], MICS_SAMPLE_RATE_MS)
            .range(MICS_RANGE_MIN, MICS_RANGE_MAX)
            .onChange(onMicChange, i);

        progState.micsLedLevel[i] = MICS_RANGE_MIN;
        progState.micsLastRead[i] = 0;
    }
}

bool isMicsLevelValid()
{
    bool isValidLevel;

    for (int i = 0; i < MICS_NUM; i++)
    {
        isValidLevel = progState.micsLedLevel[i] >= MICS_THRESHOLD_MIN &&
                       progState.micsLedLevel[i] <= MICS_THRESHOLD_MAX;

        if (!isValidLevel)
        {
            return false;
        }
    }

    return true;
}

void resetMicsLastRead()
{
    for (int i = 0; i < MICS_NUM; i++)
    {
        progState.micsLastRead[i] = 0;
    }
}

bool isMicsUpdateRecent()
{
    unsigned long now = millis();

    bool isRecent;
    unsigned long diff;

    for (int i = 0; i < MICS_NUM; i++)
    {
        if (now < progState.micsLastRead[i])
        {
            Serial.println(F("Mics :: Timer overflow"));
            resetMicsLastRead();
            return false;
        }

        diff = now - progState.micsLastRead[i];

        if (diff > (MICS_TIMER_MS * MICS_RECENT_TIMER_RATIO))
        {
            return false;
        }
    }

    return true;
}

void decreaseMicsLedLevel()
{
    for (int i = 0; i < MICS_NUM; i++)
    {
        if (progState.micsLedLevel[i] > 0)
        {
            progState.micsLedLevel[i]--;

            Serial.print(F("Mics :: #"));
            Serial.print(i);
            Serial.print(F(" :: -LED :: "));
            Serial.println(progState.micsLedLevel[i]);
        }
    }

    refreshLedsMics();
}

void onMicsPhaseComplete()
{
    Serial.println(F("Mics :: Completed"));
    openRelayMics();
    progState.isMicsPhaseComplete = true;
}

bool isMicsPhaseComplete()
{
    return progState.micsValidLevelCounter >= MICS_VALID_COUNTER_TARGET;
}

void onMicsTimer(int idx, int v, int up)
{
    if (!shouldListenToMics())
    {
        return;
    }

    if (isMicsPhaseComplete())
    {
        onMicsPhaseComplete();
        return;
    }

    if (!isMicsUpdateRecent())
    {
        if (progState.micsValidLevelCounter > 0)
        {
            progState.micsValidLevelCounter--;
            Serial.print(F("Mics :: Expired (-counter) :: "));
            Serial.println(progState.micsValidLevelCounter);
        }

        decreaseMicsLedLevel();
        return;
    }

    if (isMicsLevelValid())
    {
        progState.micsValidLevelCounter++;
        Serial.print(F("Mics :: Level valid (+counter) :: "));
        Serial.println(progState.micsValidLevelCounter);
    }
    else if (progState.micsValidLevelCounter > 0)
    {
        progState.micsValidLevelCounter--;
        Serial.print(F("Mics :: Level invalid (-counter) :: "));
        Serial.println(progState.micsValidLevelCounter);
    }
}

void initMicsTimer()
{
    micsTimer
        .begin(MICS_TIMER_MS)
        .repeat(-1)
        .onTimer(onMicsTimer)
        .start();
}

/**
 * RFID functions.
 */

void initRfid()
{
    rfidReader.begin();
}

void onRfidPhaseComplete()
{
    Serial.print(F("RFID phase complete"));
    openRelayRfid();
    progState.isRfidPhaseComplete = true;
}

void pollRfidReader()
{
    String tagId;

    tagId = rfidReader.getTagId();

    if (!tagId.length())
    {
        return;
    }

    Serial.print(F("Tag: "));
    Serial.println(tagId);

    for (int i = 0; i < NUM_VALID_TAGS; i++)
    {
        if (validTags[i].compareTo(tagId) == 0)
        {
            onRfidPhaseComplete();
            return;
        }
    }
}

/**
 * Relay functions.
 */

void lockRelayRfid()
{
    digitalWrite(RELAY_PIN_RFID, LOW);
}

void openRelayRfid()
{
    digitalWrite(RELAY_PIN_RFID, HIGH);
}

void lockRelayMics()
{
    digitalWrite(RELAY_PIN_MICS, LOW);
}

void openRelayMics()
{
    digitalWrite(RELAY_PIN_MICS, HIGH);
}

void initRelays()
{
    pinMode(RELAY_PIN_RFID, OUTPUT);
    pinMode(RELAY_PIN_MICS, OUTPUT);

    lockRelayRfid();
    lockRelayMics();
}

/**
 * Entrypoint.
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
    initServo();
    initRfid();
    initRelays();
    initMics();
    initMicsTimer();

    Serial.println(F(">> Starting Runebook program"));
}

void loop()
{
    automaton.run();

    if (shouldListenToProxSensors())
    {
        refreshLedsBook();
        refreshLedsPipes();
    }
    else if (shouldListenToRfid())
    {
        pollRfidReader();
    }
}
