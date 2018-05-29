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
char matchTag2[SIZE_TAG_ID] = "5C00CAC9633D";
char matchTag3[SIZE_TAG_ID] = "5C00CAC9633F";

void openFirstRelay(char *tag) {
  Serial.println("Opening relay 01");
  openRelay(RELAY_PIN_1);
}

void openSecondRelay(char *tag) {
  Serial.println("Opening relay 02");
  openRelay(RELAY_PIN_2);
}

void openThirdRelay(char *tag) {
  Serial.println("Opening relay 03");
  openRelay(RELAY_PIN_3);
}

void lockRelay(byte pin) {
  digitalWrite(pin, HIGH);
}

void openRelay(byte pin) {
  digitalWrite(pin, LOW);
}

void initRelays() {
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);

  lockRelay(RELAY_PIN_1);
  lockRelay(RELAY_PIN_2);
  lockRelay(RELAY_PIN_3);
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
