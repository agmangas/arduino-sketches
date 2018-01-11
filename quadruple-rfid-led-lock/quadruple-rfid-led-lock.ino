#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "limits.h"

// RX and TX pins for secondary RFID sensors
const byte SECONDARY_RFID_01_RX = 13;
const byte SECONDARY_RFID_01_TX = 12;
const byte SECONDARY_RFID_02_RX = 11;
const byte SECONDARY_RFID_02_TX = 6;
const byte SECONDARY_RFID_03_RX = 9;
const byte SECONDARY_RFID_03_TX = 5;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 4;
const uint8_t NEOPIXEL_PIN = 3;

// For SparkFun's tags, we will receive 16 bytes on every
// tag read, but throw 4 away. The 13th space will always
// be 0, since proper strings in Arduino end with 0
const int TAG_LEN = 16;
const int ID_LEN = 13;
const int ID_PRINTABLE_LEN = 12;

// Total number of RFID readers
const int NUM_READERS = 4;

// Maximum delay in milliseconds between the moment
// a tag is read and the moment the read event is
// considered invalid or too old
const unsigned long MAX_ACTIVATION_DELAY_MS = 300000;

// Digital pin to which the relay that controls the magnetic lock is connected
const byte LOCK_RELAY_PIN = 10;

// Number of milliseconds that the electronic lock must remain open
// after all tags have been correctly placed in their RFID sensors
const unsigned long LOCK_OPEN_DELAY_MS = 40000;

// Non-printable tag buffer characters
const int TAG_CHAR_STX = 2;
const int TAG_CHAR_CR = 13;
const int TAG_CHAR_LF = 10;
const int TAG_CHAR_ETX = 3;

// SoftwareSerial instances (RX, TX) connected to RFID sensors
SoftwareSerial rSerial01(SECONDARY_RFID_01_RX, SECONDARY_RFID_01_TX);
SoftwareSerial rSerial02(SECONDARY_RFID_02_RX, SECONDARY_RFID_02_TX);
SoftwareSerial rSerial03(SECONDARY_RFID_03_RX, SECONDARY_RFID_03_TX);

// Initialize the NeoPixel instance
Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Some NeoPixel colors
uint32_t colorActive = pixelStrip.Color(255, 255, 255);

// List of accepted tags for each RFID sensor
char acceptedTags[NUM_READERS][ID_LEN] = {
  "011005CD9C45",
  "011005CD9C45",
  "011005CD9C45",
  "011005CD9C45"
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
  if (readByte != TAG_CHAR_STX &&
      readByte != TAG_CHAR_CR &&
      readByte != TAG_CHAR_LF &&
      readByte != TAG_CHAR_ETX) {
    return true;
  } else {
    return false;
  }
}

/**
   Clears the main HW serial input buffer.
*/
void clearHardwareSerialInputBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

/**
   Clears the input buffers of a secondary SW serial.
*/
void clearSoftwareSerialInputBuffer(SoftwareSerial &theSerialPort) {
  while (theSerialPort.available()) {
    theSerialPort.read();
  }
}

/**
   Clears the recent tags array and the serial input buffers.
*/
void clearCurrentTagsAndBuffers() {
  for (int i = 0; i < NUM_READERS; i++) {
    memset(currentTags[i], 0, sizeof(currentTags[i]));
  }

  clearHardwareSerialInputBuffer();
  clearSoftwareSerialInputBuffer(rSerial01);
  clearSoftwareSerialInputBuffer(rSerial02);
  clearSoftwareSerialInputBuffer(rSerial03);
}

/**
   Takes a buffer of bytes and tries to retrieve the most recent tag.
*/
void findTagInBuffer(char buf[], int bufSize, char newTag[]) {
  memset(newTag, 0, ID_LEN);

  if (bufSize < TAG_LEN) {
    return;
  }

  const int stxIdx = bufSize - TAG_LEN;

  if (buf[stxIdx] != TAG_CHAR_STX) {
    return;
  }

  int counter = 0;

  for (int i = stxIdx; i < bufSize; i++) {
    if (isPrintableTagChar(buf[i])) {
      // Check if we are reading more bytes than expected
      if (counter >= (ID_LEN - 1)) {
        memset(newTag, 0, ID_LEN);
        return;
      }

      newTag[counter] = buf[i];
      counter++;
    }
  }

  // Check if the output array does not have the string length
  if (strlen(newTag) != ID_PRINTABLE_LEN) {
    memset(newTag, 0, ID_LEN);
  }
}

/**
   Reads the main RFID reader on the HW serial port.
*/
void readMainTag() {
  if (Serial.available() > 0 && Serial.peek() != TAG_CHAR_STX) {
    Serial.println("Unexpected start byte in HW serial: clearing input buffer");
    Serial.flush();

    clearHardwareSerialInputBuffer();
  }

  if (Serial.available() < TAG_LEN) {
    return;
  }

  char newTag[ID_LEN];

  int bufSize = Serial.available();
  char buf[bufSize];

  for (int i = 0; i < bufSize; i++) {
    buf[i] = Serial.read();
  }

  findTagInBuffer(buf, bufSize, newTag);

  if (strlen(newTag) == ID_PRINTABLE_LEN) {
    Serial.print("Read main tag: ");
    Serial.print(newTag);
    Serial.println();
    Serial.flush();

    for (int k = 0; k < ID_LEN - 1; k++) {
      currentTags[0][k] = newTag[k];
    }

    currentTagsMillis[0] = millis();

    memset(currentTags[1], 0, sizeof(currentTags[1]));
    memset(currentTags[2], 0, sizeof(currentTags[2]));
    memset(currentTags[3], 0, sizeof(currentTags[3]));

    clearSoftwareSerialInputBuffer(rSerial01);
    clearSoftwareSerialInputBuffer(rSerial02);
    clearSoftwareSerialInputBuffer(rSerial03);

    turnOffPixels();

    if (isTagEqual(newTag, acceptedTags[0]) == true) {
      turnOnPixel(0);
    }
  }
}

/**
   Reads a secondary RFID reader on a SW serial port.
*/
void readSecondaryTag(SoftwareSerial &theSerialPort, int portIndex) {
  if (theSerialPort.available() > 0 && theSerialPort.peek() != TAG_CHAR_STX) {
    Serial.print("Unexpected start byte in SW serial #");
    Serial.print(portIndex);
    Serial.print(": clearing input buffer");
    Serial.println();
    Serial.flush();

    clearSoftwareSerialInputBuffer(theSerialPort);
  }

  if (theSerialPort.available() < TAG_LEN) {
    return;
  }

  char newTag[ID_LEN];

  int bufSize = theSerialPort.available();
  char buf[bufSize];

  for (int i = 0; i < bufSize; i++) {
    buf[i] = theSerialPort.read();
  }

  findTagInBuffer(buf, bufSize, newTag);

  if (strlen(newTag) == ID_PRINTABLE_LEN) {
    Serial.print("Read tag: ");
    Serial.print(newTag);
    Serial.print(" in port #");
    Serial.print(portIndex);
    Serial.println();
    Serial.flush();

    for (int k = 0; k < ID_LEN - 1; k++) {
      currentTags[portIndex][k] = newTag[k];
    }

    currentTagsMillis[portIndex] = millis();

    if (isTagDefined(0) &&
        isTagEqual(newTag, acceptedTags[portIndex]) == true) {
      turnOnPixel(portIndex);
    }
  }
}

/**
   Returns true if the tag with the given index is defined.
*/
bool isTagDefined(int portIdx) {
  return strlen(currentTags[portIdx]) == ID_PRINTABLE_LEN;
}

/**
   Checks the current status and updates the SW serial port that is listening.
*/
void updateListenerPort() {
  if (isTagDefined(0) && isTagDefined(1) && isTagDefined(2)) {
    if (rSerial03.isListening() == false) {
      Serial.println("Listening on port #3");
      Serial.flush();
      rSerial03.listen();
    }
  } else if (isTagDefined(0) && isTagDefined(1)) {
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
   Turns all the LEDs off.
*/
void turnOffPixels() {
  Serial.println("Turning all the LEDs off");
  Serial.flush();

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 0, 0, 0);
  }

  pixelStrip.show();
}

/**
   Turns off the given LED.
*/
void turnOffPixel(int idx) {
  Serial.print("Turning off LED #");
  Serial.print(idx);
  Serial.println();
  Serial.flush();

  pixelStrip.setPixelColor(idx, 0, 0, 0);
  pixelStrip.show();
}

/**
  Turns on the given LED with the predefined color.
*/
void turnOnPixel(int idx) {
  Serial.print("Turning on LED #");
  Serial.print(idx);
  Serial.println();
  Serial.flush();

  pixelStrip.setPixelColor(idx, colorActive);
  pixelStrip.show();
}

/**
   Start a sequence where all LEDs pulsate for a given amount of time.
*/
void pulsatePixels(unsigned long durationMs) {
  unsigned long ini = millis();

  Serial.print("Starting pulsating sequence at: ");
  Serial.print(ini);
  Serial.println();
  Serial.flush();

  uint8_t val = 0;
  uint8_t valStep = 5;
  bool isRising = true;

  while (true) {
    unsigned long now = millis();

    if (isRising) {
      val = val + valStep;
    } else {
      val = val - valStep;
    }

    if (val >= 255) {
      val = 255;
      isRising = false;
    } else if (val <= 0) {
      val = 0;
      isRising = true;
    }

    for (int i = 0; i < NEOPIXEL_NUM; i++) {
      pixelStrip.setPixelColor(i, val, val, val);
    }

    pixelStrip.show();

    if (now < ini || ((now - ini) > durationMs)) {
      break;
    }

    delay(30);
  }

  turnOffPixels();

  Serial.print("Ending pulsating sequence at: ");
  Serial.print(millis());
  Serial.println();
  Serial.flush();
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
  rSerial03.begin(9600);

  delay(1000);

  Serial.println("Starting RFID electronic lock program");
  Serial.flush();

  updateListenerPort();

  pixelStrip.begin();
  pixelStrip.setBrightness(200);
  pixelStrip.show();
}

void loop() {
  readMainTag();
  updateListenerPort();
  readSecondaryTag(rSerial01, 1);
  updateListenerPort();
  readSecondaryTag(rSerial02, 2);
  updateListenerPort();
  readSecondaryTag(rSerial03, 3);

  if (tagsAreActivated() == true) {
    Serial.print("All tags have been placed. Opening for (ms): ");
    Serial.print(LOCK_OPEN_DELAY_MS);
    Serial.println();
    Serial.flush();

    digitalWrite(LOCK_RELAY_PIN, LOW);
    pulsatePixels(LOCK_OPEN_DELAY_MS);

    Serial.println("Opening time has finished: enabling lock");
    Serial.flush();

    clearCurrentTagsAndBuffers();
    turnOffPixels();
    digitalWrite(LOCK_RELAY_PIN, HIGH);
  }
}
