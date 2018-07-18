#include <SoftwareSerial.h>
#include <SerialRFID.h>

const byte RX_PIN = 12;
const byte TX_PIN = 13;

const byte RELAY_PIN_1 = 11;
const byte RELAY_PIN_2 = 10;
const byte RELAY_PIN_3 = 9;

SoftwareSerial sSerial(RX_PIN, TX_PIN);
SerialRFID rfid(sSerial);

char matchTag1[SIZE_TAG_ID] = "5C00CAC9633C";
char matchTag2[SIZE_TAG_ID] = "5C00CB17C444";
char matchTag3[SIZE_TAG_ID] = "5C00CB17DC5C";

void openFirstRelay(char *tag) {
  Serial.println("Opening relay 01");
  digitalWrite(RELAY_PIN_1, HIGH);
}

void openSecondRelay(char *tag) {
  Serial.println("Opening relay 02");
  digitalWrite(RELAY_PIN_2, HIGH);
}

void openThirdRelay(char *tag) {
  Serial.println("Opening relay 03 (Room of Storms)");
  digitalWrite(RELAY_PIN_3, LOW);
}

void initRelays() {
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);

  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);
  digitalWrite(RELAY_PIN_3, HIGH);
}

void initRFID() {
  rfid.onTag(openFirstRelay, matchTag1);
  rfid.onTag(openSecondRelay, matchTag2);
  rfid.onTag(openThirdRelay, matchTag3);
}

void setup() {
  Serial.begin(9600);
  sSerial.begin(9600);

  initRelays();
  initRFID();

  Serial.println(">> Starting Earth Globe program");
}

void loop() {
  rfid.run();
}
