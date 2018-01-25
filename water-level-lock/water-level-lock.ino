const byte WATER_SENSOR_PIN = 0;
const byte RELAY_PIN = 10;
const int LEVEL_THRESHOLD = 200;
const unsigned long LOOP_DELAY_MS = 2000;

char printBuffer[128];
bool isLevelOverThreshold = false;

int readWaterLevelSensor() {
  return analogRead(WATER_SENSOR_PIN);
}

int detectWaterLevelSignalEdge() {
  int level = readWaterLevelSensor();

  sprintf(printBuffer, "Water level: %d\n", level);
  Serial.print(printBuffer);

  if (level > LEVEL_THRESHOLD && isLevelOverThreshold == false) {
    Serial.println("### Water level: ON");
    Serial.flush();
    isLevelOverThreshold = true;
    return 1;
  } else if (level <= LEVEL_THRESHOLD && isLevelOverThreshold == true) {
    Serial.println("### Water level: OFF");
    Serial.flush();
    isLevelOverThreshold = false;
    return -1;
  }

  return 0;
}

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println(">>> Starting water level program");
  Serial.flush();
}

void loop() {
  int edge = detectWaterLevelSignalEdge();

  if (edge == 1) {
    Serial.println("### Opening relay");
    Serial.flush();
    digitalWrite(RELAY_PIN, LOW);
  } else if (edge == -1) {
    Serial.println("### Closing relay");
    Serial.flush();
    digitalWrite(RELAY_PIN, HIGH);
  }

  delay(LOOP_DELAY_MS);
}
