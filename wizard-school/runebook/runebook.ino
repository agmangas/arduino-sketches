#include <limits.h>

/**
  Shortest paths in the LED matrix.
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
  Functions to update the path buffer with the shortest path.
*/

void emptyPathBuffers() {
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

void updatePathBuffers(int start, int finish) {
  emptyPathBuffers();

  int matchIdx = -1;

  bool isMatchAsc;
  bool isMatchDesc;

  for (int i = 0; i < PATHS_SIZE; i++) {
    isMatchAsc = PATHS[i][0] == start &&
                 PATHS[i][PATHS_ITEM_LEN - 1] == finish;

    isMatchDesc = PATHS[i][0] == finish &&
                  PATHS[i][PATHS_ITEM_LEN - 1] == start;

    if (isMatchAsc || isMatchDesc) {
      matchIdx = i;
      break;
    }
  }

  if (matchIdx == -1) {
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
   Entrypoint.
*/

void setup() {
  emptyPathBuffers();
}

void loop() {
}
