#include <SerialRFID.h>
#include <SoftwareSerial.h>
#include "limits.h"

/**
  Structs.
*/

typedef struct programState {
  bool relayOpened;
} ProgramState;

ProgramState progState = {
  .relayOpened = false
};

/**
   Pins.
*/

const byte SECONDARY_RFID_01_RX = 9;
const byte SECONDARY_RFID_01_TX = 8;
const byte SECONDARY_RFID_02_RX = 7;
const byte SECONDARY_RFID_02_TX = 6;
const byte SECONDARY_RFID_03_RX = 5;
const byte SECONDARY_RFID_03_TX = 4;

const byte LOCK_RELAY_PIN = 10;

/**
   Configuration constants.
*/

const int NUM_READERS = 4;
const unsigned long MAX_ACTIVATION_DELAY_MS = 3600000;

/**
   SoftwareSerial instances linked with the RFID sensors.
*/

SoftwareSerial sSerial01(SECONDARY_RFID_01_RX, SECONDARY_RFID_01_TX);
SoftwareSerial sSerial02(SECONDARY_RFID_02_RX, SECONDARY_RFID_02_TX);
SoftwareSerial sSerial03(SECONDARY_RFID_03_RX, SECONDARY_RFID_03_TX);

/**
   SerialRFID instances.
*/

SerialRFID rfidMain(Serial);
SerialRFID rfid01(sSerial01);
SerialRFID rfid02(sSerial02);
SerialRFID rfid03(sSerial03);

/**
   List of accepted tags for each RFID sensor.
*/

char acceptedTags[NUM_READERS][SIZE_TAG_ID] = {
  "011005CD9B42",
  "011005CD9C45",
  "011005CE79A3",
  "011005CDA079"
};

/**
   Most recent tags and timestamps for each RFID sensor.
*/

char currentTags[NUM_READERS][SIZE_TAG_ID];
unsigned long currentTagsMillis[NUM_READERS];

/**
   Functions to deal with the serial streams and tag buffers.
*/

void clearStream(Stream &stream) {
  while (stream.available()) {
    stream.read();
  }
}

void clearStreamsAndBuffers() {
  for (int i = 0; i < NUM_READERS; i++) {
    memset(currentTags[i], 0, sizeof(currentTags[i]));
  }

  clearStream(Serial);
  clearStream(sSerial01);
  clearStream(sSerial02);
  clearStream(sSerial03);
}

void updateListenerPort() {
  if (isTagDefined(0) && isTagDefined(1) && isTagDefined(2)) {
    if (sSerial03.isListening() == false) {
      Serial.println("Listening on port #3");
      Serial.flush();
      sSerial03.listen();
    }
  } else if (isTagDefined(0) && isTagDefined(1)) {
    if (sSerial02.isListening() == false) {
      Serial.println("Listening on port #2");
      Serial.flush();
      sSerial02.listen();
    }
  } else {
    if (sSerial01.isListening() == false) {
      Serial.println("Listening on port #1");
      Serial.flush();
      sSerial01.listen();
    }
  }
}

/**
   Functions to read the RFID sensors.
*/

void readMainTag() {
  char newTag[SIZE_TAG_ID];

  if (rfidMain.readTag(newTag, sizeof(newTag))) {
    Serial.print("Read main tag: ");
    Serial.print(newTag);
    Serial.println();
    Serial.flush();

    for (int k = 0; k < LEN_TAG_ID; k++) {
      currentTags[0][k] = newTag[k];
    }

    currentTagsMillis[0] = millis();

    memset(currentTags[1], 0, sizeof(currentTags[1]));
    memset(currentTags[2], 0, sizeof(currentTags[2]));
    memset(currentTags[3], 0, sizeof(currentTags[3]));

    clearStream(sSerial01);
    clearStream(sSerial02);
    clearStream(sSerial03);
  }
}

void readSecondaryTag(SerialRFID &theRfid, int portIndex) {
  char newTag[SIZE_TAG_ID];

  if (theRfid.readTag(newTag, sizeof(newTag))) {
    Serial.print("Read tag: ");
    Serial.print(newTag);
    Serial.print(" in port #");
    Serial.print(portIndex);
    Serial.println();
    Serial.flush();

    for (int k = 0; k < LEN_TAG_ID; k++) {
      currentTags[portIndex][k] = newTag[k];
    }

    currentTagsMillis[portIndex] = millis();
  }
}

/**
   Functions to check the status of the tag buffers.
*/

bool isTagDefined(int portIdx) {
  return strlen(currentTags[portIdx]) == LEN_TAG_ID;
}

bool tagsAreActivated() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_READERS; i++) {
    if (strlen(currentTags[i]) == 0) {
      // This RFID sensor has not seen any tags yet
      return false;
    }

    if (SerialRFID::isEqualTag(currentTags[i], acceptedTags[i]) == false) {
      // Incorrect tag
      return false;
    }

    unsigned long diffMs;

    if (currentTagsMillis[i] > now) {
      diffMs = (ULONG_MAX - currentTagsMillis[i]) + now;
      Serial.println("Clock overflow when checking for tag age in ms");
      Serial.flush();
    } else {
      diffMs = now - currentTagsMillis[i];
    }

    if (diffMs > MAX_ACTIVATION_DELAY_MS) {
      // The most recent tag is valid but was read too long ago
      return false;
    }
  }

  return true;
}

/**
   Entrypoint.
*/

void setup() {
  pinMode(LOCK_RELAY_PIN, OUTPUT);
  digitalWrite(LOCK_RELAY_PIN, LOW);

  Serial.begin(9600);
  sSerial01.begin(9600);
  sSerial02.begin(9600);
  sSerial03.begin(9600);

  Serial.println(">> Starting RFID electronic lock program");
  Serial.flush();

  updateListenerPort();
}

void loop() {
  updateListenerPort();

  readMainTag();
  readSecondaryTag(rfid01, 1);
  readSecondaryTag(rfid02, 2);
  readSecondaryTag(rfid03, 3);

  if (tagsAreActivated() == true && progState.relayOpened == false) {
    Serial.print("All tags have been placed. Opening lock");
    Serial.println();
    Serial.flush();

    digitalWrite(LOCK_RELAY_PIN, HIGH);
    progState.relayOpened == true;
  }
}
