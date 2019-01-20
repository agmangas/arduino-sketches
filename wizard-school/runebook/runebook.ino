#include <limits.h>

/**
  Shortest paths in the LED matrix
*/

const byte PATHS_SIZE = 18;
const byte PATHS_ITEM_LEN = 4;

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

void setup() {
}

void loop() {
}
