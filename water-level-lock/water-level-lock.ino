const byte WATER_SENSOR_PIN = 0;
const int LEVEL_THRESHOLD = 150;
const int CHANGE_THRESHOLD = 10;
const unsigned long LOOP_DELAY_MS = 2000;

int levelPrev = 0;
int levelCurr = NULL;
int levelChanged = NULL;
char printBuffer[128];
bool currentStatus = false;

void updateWaterLevelVars() {
  int level = analogRead(WATER_SENSOR_PIN);

  levelCurr = level;

  if (((levelPrev >= levelCurr) && ((levelPrev - levelCurr) > CHANGE_THRESHOLD)) ||
      ((levelPrev < levelCurr) && ((levelCurr - levelPrev) > CHANGE_THRESHOLD))) {
    levelChanged = levelCurr;
    levelPrev = levelCurr;
  }
}

int getWaterLevelIfChanged() {
  updateWaterLevelVars();

  int ret;

  if (levelChanged != NULL) {
    ret = levelChanged;
    levelChanged = NULL;
  } else {
    ret = NULL;
  }

  return ret;
}

int detectLevelStatusEdge() {
  int level = getWaterLevelIfChanged();

  if (level != NULL) {
    sprintf(printBuffer, "ADC%d level is %d\n", WATER_SENSOR_PIN, level);
    Serial.print(printBuffer);

    if (level > LEVEL_THRESHOLD && currentStatus == false) {
      Serial.println("##### Water level: ON");
      Serial.flush();
      currentStatus = true;
      return 1;
    } else if (level <= LEVEL_THRESHOLD && currentStatus == true) {
      Serial.println("##### Water level: OFF");
      Serial.flush();
      currentStatus = false;
      return -1;
    }
  }

  return 0;
}

void setup() {
  Serial.begin(9600);

  Serial.println(">>>>> Starting water level program");
  Serial.flush();
}

void loop() {
  detectLevelStatusEdge();
  delay(LOOP_DELAY_MS);
}
