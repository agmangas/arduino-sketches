#include <SerialRFID.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
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

const byte PIN_RELAY = 10;

/**
   Configuration constants.
*/

const int NUM_READERS = 3;
const unsigned long MAX_ACTIVATION_DELAY_MS = 3600000;

/**
   LED constants.
*/

const int DEFAULT_BRIGHTNESS = 100;

const uint16_t NEOPIXEL_NUMS[NUM_READERS] = {
  1, 1, 1
};

const uint8_t NEOPIXEL_PINS[NUM_READERS] = {
  3, 4, 5
};

const uint32_t LED_COLORS[NUM_READERS] = {
  Adafruit_NeoPixel::Color(255, 0, 0),
  Adafruit_NeoPixel::Color(0, 255, 0),
  Adafruit_NeoPixel::Color(0, 0, 255)
};

Adafruit_NeoPixel pixelStrips[NUM_READERS];

/**
   SoftwareSerial instances linked with the RFID sensors.
*/

SoftwareSerial sSerial01(SECONDARY_RFID_01_RX, SECONDARY_RFID_01_TX);
SoftwareSerial sSerial02(SECONDARY_RFID_02_RX, SECONDARY_RFID_02_TX);

/**
   SerialRFID instances.
*/

SerialRFID rfidMain(Serial);
SerialRFID rfid01(sSerial01);
SerialRFID rfid02(sSerial02);

/**
   List of accepted tags for each RFID sensor.
*/

char acceptedTags[NUM_READERS][SIZE_TAG_ID] = {
  "011005CD9B42",
  "011005CD9C45",
  "011005CE79A3"
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
}

void updateListenerPort() {
  if (isTagDefined(0) && isTagDefined(1)) {
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

    clearStream(sSerial01);
    clearStream(sSerial02);
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

bool isTagActivated(int idx) {
  unsigned long now = millis();

  if (strlen(currentTags[idx]) == 0) {
    // This RFID sensor has not seen any tags yet
    return false;
  }

  if (SerialRFID::isEqualTag(currentTags[idx], acceptedTags[idx]) == false) {
    // Incorrect tag
    return false;
  }

  unsigned long diffMs;

  if (currentTagsMillis[idx] > now) {
    diffMs = (ULONG_MAX - currentTagsMillis[idx]) + now;
    Serial.println("Clock overflow when checking tag age");
  } else {
    diffMs = now - currentTagsMillis[idx];
  }

  if (diffMs > MAX_ACTIVATION_DELAY_MS) {
    // The most recent tag is valid but was read too long ago
    return false;
  }

  return true;
}

bool tagsAreActivated() {
  for (int i = 0; i < NUM_READERS; i++) {
    if (isTagActivated(i) == false) {
      return false;
    }
  }

  return true;
}

/**
   LED functions.
*/

void initLedStrips() {
  for (int i = 0; i < NUM_READERS; i++) {
    pixelStrips[i] = Adafruit_NeoPixel(NEOPIXEL_NUMS[i], NEOPIXEL_PINS[i], NEO_GRB + NEO_KHZ800);
    pixelStrips[i].begin();
    pixelStrips[i].setBrightness(DEFAULT_BRIGHTNESS);
    pixelStrips[i].clear();
    pixelStrips[i].show();
  }
}

void updateLedStrips() {
  for (int i = 0; i < NUM_READERS; i++) {
    if (isTagActivated(i) == true) {
      for (int j = 0; j < NEOPIXEL_NUMS[i]; j++) {
        pixelStrips[i].setPixelColor(j, LED_COLORS[i]);
      }
    } else {
      pixelStrips[i].clear();
    }

    pixelStrips[i].show();
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
  if (tagsAreActivated() == true && progState.relayOpened == false) {
    Serial.println("All tags have been placed: Opening relay");
    openRelay();
  } else if (tagsAreActivated() == false && progState.relayOpened == true) {
    Serial.println("Invalid tags: Locking relay");
    lockRelay();
  }
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);
  sSerial01.begin(9600);
  sSerial02.begin(9600);

  lockRelay();
  updateListenerPort();
  initLedStrips();

  Serial.println(">> Starting RFID electronic lock program");
  Serial.flush();
}

void loop() {
  updateListenerPort();

  readMainTag();
  readSecondaryTag(rfid01, 1);
  readSecondaryTag(rfid02, 2);

  updateLedStrips();

  checkStatusToUpdateRelay();
}
