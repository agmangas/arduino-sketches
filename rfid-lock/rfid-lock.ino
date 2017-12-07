#include <SoftwareSerial.h>
#include "limits.h"

// RX and TX pins for secondary RFID sensors
const byte SECONDARY_RFID_01_RX = 13;
const byte SECONDARY_RFID_01_TX = 12;
const byte SECONDARY_RFID_02_RX = 11;
const byte SECONDARY_RFID_02_TX = 9;

// For SparkFun's tags, we will receive 16 bytes on every
// tag read, but throw 4 away. The 13th space will always
// be 0, since proper strings in Arduino end with 0
const int TAG_LEN = 16;
const int ID_LEN = 13;
const int ID_PRINTABLE_LEN = 12;

// Total number of RFID readers
const int NUM_READERS = 3;

// Maximum delay in milliseconds between the moment
// a tag is read and the moment the read event is
// considered invalid or too old
const unsigned long MAX_ACTIVATION_DELAY_MS = 300000;

// Digital pin to which the relay that controls the magnetic lock is connected
const byte LOCK_RELAY_PIN = 10;

// Number of milliseconds that the electronic lock must remain open
// after all tags have been correctly placed in their RFID sensors
const unsigned long LOCK_OPEN_DELAY_MS = 120000;

// SoftwareSerial instances (RX, TX) connected to RFID sensors
SoftwareSerial rSerial01(SECONDARY_RFID_01_RX, SECONDARY_RFID_01_TX);
SoftwareSerial rSerial02(SECONDARY_RFID_02_RX, SECONDARY_RFID_02_TX);

// List of accepted tags for each RFID sensor
char acceptedTags[NUM_READERS][ID_LEN] = {
  "111111111111",
  "444444444444",
  "555555555555"
};

// Most recent tags and timestamps for each RFID sensor
char currentTags[NUM_READERS][ID_LEN];
unsigned long currentTagsMillis[NUM_READERS];

/**
   Returns true if the given byte is a printable tag character.
   Ignored characters include the first one (2, STX, start of text) and the last three:
   ASCII 13, CR/carriage return.
   ASCII 10, LF/linefeed.
   ASCII 3, ETX/end of text.
*/
boolean isPrintableTagChar(int readByte) {
  if (readByte != 2 && readByte != 13 && readByte != 10 && readByte != 3) {
    return true;
  } else {
    return false;
  }
}

/**
   Clears the main HW serial input buffer.
*/
void clearMainSerialInputBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

/**
   Clears the input buffers of all secondary SW serials.
*/
void clearSecondarySerialsInputBuffer() {
  while (rSerial01.available()) {
    rSerial01.read();
  }

  while (rSerial02.available()) {
    rSerial02.read();
  }
}

/**
   Clears the recent tags array and the serial input buffers.
*/
void clearCurrentTagsAndBuffers() {
  for (int i = 0; i < NUM_READERS; i++) {
    memset(currentTags[i], 0, sizeof(currentTags[i]));
  }

  clearMainSerialInputBuffer();
  clearSecondarySerialsInputBuffer();
}

/**
   Reads the main RFID reader on the HW serial port.
*/
void readMainTag() {
  char newTag[ID_LEN];

  if (Serial.available() == TAG_LEN) {
    int i = 0;
    int readByte;

    while (Serial.available()) {
      readByte = Serial.read();

      if (isPrintableTagChar(readByte)) {
        newTag[i] = readByte;
        i++;
      }
    }
  } else if (Serial.available() > TAG_LEN) {
    Serial.println("Too many bytes in HW serial: clearing input buffer");
    Serial.flush();

    clearMainSerialInputBuffer();
  }

  if (strlen(newTag) == ID_PRINTABLE_LEN) {
    Serial.print("Read main tag: ");
    Serial.print(newTag);
    Serial.println();
    Serial.flush();

    strncpy(newTag, currentTags[0], ID_LEN - 1);
    currentTagsMillis[0] = millis();

    memset(currentTags[1], 0, sizeof(currentTags[1]));
    memset(currentTags[2], 0, sizeof(currentTags[2]));

    clearSecondarySerialsInputBuffer();
  }
}

/**
   Reads a secondary RFID reader on a SW serial port.
*/
void readSecondaryTag(SoftwareSerial &theSerialPort, int portIndex) {
  char newTag[ID_LEN];

  if (theSerialPort.available() == TAG_LEN) {
    int i = 0;
    int readByte;

    while (theSerialPort.available()) {
      readByte = theSerialPort.read();

      if (isPrintableTagChar(readByte)) {
        newTag[i] = readByte;
        i++;
      }
    }
  } else if (theSerialPort.available() > TAG_LEN) {
    Serial.print("Too many bytes in SW serial #");
    Serial.print(portIndex);
    Serial.print(": clearing input buffer");
    Serial.println();
    Serial.flush();

    while (theSerialPort.available()) {
      theSerialPort.read();
    }
  }

  if (strlen(newTag) == ID_PRINTABLE_LEN) {
    Serial.print("Read tag: ");
    Serial.print(newTag);
    Serial.print(" in port #");
    Serial.print(portIndex);
    Serial.println();
    Serial.flush();

    strncpy(newTag, currentTags[portIndex], ID_LEN - 1);
    currentTagsMillis[portIndex] = millis();
  }
}

/**
   Checks the current status and updates the SW serial port that is listening.
*/
void updateListenerPort() {
  bool hasMain = strlen(currentTags[0]) == ID_PRINTABLE_LEN;
  bool hasFirstSecondary = strlen(currentTags[1]) == ID_PRINTABLE_LEN;

  if (hasMain && hasFirstSecondary) {
    if (rSerial02.isListening() == false) {
      Serial.println("Listening on port #2");
      Serial.flush();
      rSerial02.listen();
    }
  } else {
    if (rSerial01.isListening() == false) {
      Serial.println("Listening on port #1");
      Serial.flush();
      rSerial01.listen();
    }
  }
}

/**
   This function steps through both newTag and one of the known tags.
   If there is a mismatch anywhere in the tag, it will return 0,
   but if every character in the tag is the same, it returns 1.
*/
bool isTagEqual(char nTag[], char oTag[]) {
  if (strlen(nTag) != ID_PRINTABLE_LEN || strlen(oTag) != ID_PRINTABLE_LEN) {
    return false;
  }

  for (int i = 0; i < ID_LEN; i++) {
    if (nTag[i] != oTag[i]) {
      return false;
    }
  }

  return true;
}

/**
   Returns true if all RFID sensors have read an accepted tag recently.
*/
bool tagsAreActivated() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_READERS; i++) {
    if (strlen(currentTags[i]) == 0) {
      // This RFID sensor has not seen any tags yet
      return false;
    }

    if (isTagEqual(currentTags[i], acceptedTags[i]) == false) {
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

void setup() {
  pinMode(LOCK_RELAY_PIN, OUTPUT);
  digitalWrite(LOCK_RELAY_PIN, HIGH);

  Serial.begin(9600);
  rSerial01.begin(9600);
  rSerial02.begin(9600);

  delay(1000);

  updateListenerPort();

  Serial.println("Hello! I'm an RFID electric lock program");
  Serial.flush();
}

void loop() {
  readMainTag();
  updateListenerPort();
  readSecondaryTag(rSerial01, 1);
  updateListenerPort();
  readSecondaryTag(rSerial02, 2);

  if (tagsAreActivated() == true) {
    Serial.print("All tags have been placed. Opening for (ms): ");
    Serial.print(LOCK_OPEN_DELAY_MS);
    Serial.println();
    Serial.flush();

    digitalWrite(LOCK_RELAY_PIN, LOW);
    delay(LOCK_OPEN_DELAY_MS);

    Serial.println("Opening time has finished: enabling lock");
    Serial.flush();

    clearCurrentTagsAndBuffers();
    digitalWrite(LOCK_RELAY_PIN, HIGH);
  }
}
