#include "rdm630.h"

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
   Relay
*/

const byte PIN_RELAY = 11;

/**
   RFID modules
*/

const byte NUM_READERS = 4;

RDM6300 rfid01(2, 3);
RDM6300 rfid02(4, 5);
RDM6300 rfid03(6, 7);
RDM6300 rfid04(8, 9);

RDM6300 rfidReaders[NUM_READERS] = {
  rfid01,
  rfid02,
  rfid03,
  rfid04
};

unsigned int readerEmptyCount[NUM_READERS] = {
  0, 0, 0, 0
};

const unsigned int EMPTY_TOLERANCE = 1;

String currentTags[NUM_READERS];

String validTags[NUM_READERS] = {
  "1D00277B1300",
  "1D00277B1400",
  "1D00277B1500",
  "1D00277B1600"
};

/**
   RFID modules functions
*/

void initRfidReaders() {
  for (int i = 0; i < NUM_READERS; i++) {
    rfidReaders[i].begin();
  }
}

void pollRfidReaders() {
  String tagId;

  for (int i = 0; i < NUM_READERS; i++) {
    tagId = rfidReaders[i].getTagId();

    if (tagId.length()) {
      readerEmptyCount[i] = 0;
    } else if (readerEmptyCount[i]  <= EMPTY_TOLERANCE) {
      readerEmptyCount[i] += 1;
    }

    currentTags[i] = tagId;
  }
}

bool isTagDefined(int idx) {
  return currentTags[idx].length() > 0;
}

bool areCurrentTagsValid() {
  for (int i = 0; i < NUM_READERS; i++) {
    if (!isTagDefined(i)) {
      return false;
    }

    if (validTags[i].compareTo(currentTags[i]) != 0) {
      return false;
    }
  }

  return true;
}

void printCurrentTags() {
  Serial.print("## Current tags :: ");
  Serial.println(millis());

  for (int i = 0; i < NUM_READERS; i++) {
    Serial.print(i);
    Serial.print(" :: ");
    Serial.println(currentTags[i]);
  }
}

/**
   Relay functions
*/

void lockRelay() {
  digitalWrite(PIN_RELAY, LOW);
  progState.relayOpened = false;
}

void openRelay() {
  digitalWrite(PIN_RELAY, HIGH);
  progState.relayOpened = true;
}

void initRelay() {
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

void checkStatusToUpdateRelay() {
  if (areCurrentTagsValid() == true && progState.relayOpened == false) {
    Serial.println("Opening relay");
    openRelay();
  } else if (areCurrentTagsValid() == false && progState.relayOpened == true) {
    Serial.println("Locking relay");
    lockRelay();
  }
}

/**
   Entrypoint
*/

void setup() {
  Serial.begin(9600);

  initRfidReaders();
  initRelay();

  Serial.println(">> Starting frames lock program");
}

void loop() {
  pollRfidReaders();
  printCurrentTags();
  checkStatusToUpdateRelay();
}

