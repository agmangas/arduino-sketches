#include <limits.h>

/**
  LED matrix pathfinding
*/

const int MATRIX_SIZE = 3;

const int ADJ_MATRIX[MATRIX_SIZE][MATRIX_SIZE] = {
  {1, 1, 1},
  {1, 1, 1},
  {1, 1, 1}
};

const int CURR_PATH_SIZE = MATRIX_SIZE * MATRIX_SIZE;
const int CURR_PATH_NULL = -1;

int pathBuffer[CURR_PATH_SIZE];

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

/**
   Based on the implementation from:
   http://scanftree.com/Data_Structure/dijkstra's-algorithm
*/
void updateDijkstraPath(int startNode, int endNode) {
  for (int pIdx = 0; pIdx < CURR_PATH_SIZE; pIdx++) {
    pathBuffer[pIdx] = CURR_PATH_NULL;
  }

  int cost[MATRIX_SIZE][MATRIX_SIZE];
  int distance[MATRIX_SIZE];
  int pred[MATRIX_SIZE];
  int visited[MATRIX_SIZE];
  int count;
  int minDistance;
  int nextNode;
  int i;
  int j;
  int pathCounter;

  for (i = 0; i < MATRIX_SIZE; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      if (ADJ_MATRIX[i][j] == 0) {
        cost[i][j] = INT_MAX;
      } else {
        cost[i][j] = ADJ_MATRIX[i][j];
      }
    }
  }

  for (i = 0; i < MATRIX_SIZE; i++) {
    distance[i] = cost[startNode][i];
    pred[i] = startNode;
    visited[i] = 0;
  }

  distance[startNode] = 0;
  visited[startNode] = 1;
  count = 1;

  while (count < MATRIX_SIZE - 1) {
    minDistance = INT_MAX;

    for (i = 0; i < MATRIX_SIZE; i++) {
      if (distance[i] < minDistance && !visited[i]) {
        minDistance = distance[i];
        nextNode = i;
      }
    }

    visited[nextNode] = 1;

    for (i = 0; i < MATRIX_SIZE; i++) {
      if (!visited[i]) {
        if (minDistance + cost[nextNode][i] < distance[i]) {
          distance[i] = minDistance + cost[nextNode][i];
          pred[i] = nextNode;
        }
      }
    }

    count++;
  }

  printf(
    "Distance %d -> %d = %d\n",
    startNode, endNode, distance[endNode]);

  j = endNode;
  pathCounter = 0;
  pathBuffer[pathCounter] = endNode;

  do {
    j = pred[j];
    pathCounter++;
    pathBuffer[pathCounter] = j;
  } while (j != startNode);

  reverseRange(pathBuffer, 0, pathCounter);

  printf("Path ");

  for (int pIdx = 0; pIdx < CURR_PATH_SIZE; pIdx++) {
    if (pathBuffer[pIdx] == CURR_PATH_NULL) {
      break;
    }

    printf("-> %d ", pathBuffer[pIdx]);
  }

  printf("\n");
}

void setup() {
  // put your setup code here, to run once:
}

void loop() {
  updateDijkstraPath(0, 2);
}
