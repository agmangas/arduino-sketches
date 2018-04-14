// Constants that enumerate the possible results when polling the RFID reader
#define NO_TAG 1
#define UNKNOWN_TAG 2
#define VALID_TAG_FIRST 3
#define VALID_TAG_SECOND 4

// Each tag is 16 bytes but the last four are to be discarded.
const int TAG_LEN = 16;
const int ID_LEN = 13;
const int ID_PRINTABLE_LEN = 12;

// IDs of valid tags
char acceptedTagFirst[ID_LEN] = "5C00CADBC28F";
char acceptedTagSecond[ID_LEN] = "5C00CADBA1EC";

// Digital pins that are connected to the Audio FX board
const byte PIN_AUDIO_TAG_FIRST = 8;
const byte PIN_AUDIO_TAG_SECOND = 9;

// Non-printable tag buffer characters
const int TAG_CHAR_STX = 2;
const int TAG_CHAR_CR = 13;
const int TAG_CHAR_LF = 10;
const int TAG_CHAR_ETX = 3;

/**
   Returns true if both tags are equal.
*/
bool isTagEqual(char nTag[], char oTag[]) {
  if (strlen(nTag) != ID_PRINTABLE_LEN ||
      strlen(oTag) != ID_PRINTABLE_LEN) {
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
   Takes a buffer of bytes and tries to retrieve the most recent tag.
   Leaves the result in newTag,
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
byte readMainTag() {
  if (Serial.available() > 0 && Serial.peek() != TAG_CHAR_STX) {
    Serial.println("Unexpected start byte in HW serial: clearing input buffer");
    Serial.flush();

    clearHardwareSerialInputBuffer();
  }

  if (Serial.available() < TAG_LEN) {
    return NO_TAG;
  }

  char newTag[ID_LEN];

  int bufSize = Serial.available();
  char buf[bufSize];

  for (int i = 0; i < bufSize; i++) {
    buf[i] = Serial.read();
  }

  findTagInBuffer(buf, bufSize, newTag);

  if (strlen(newTag) == ID_PRINTABLE_LEN) {
    Serial.print("Main reader: ");
    Serial.print(newTag);
    Serial.println();
    Serial.flush();

    if (isTagEqual(newTag, acceptedTagFirst)) {
      return VALID_TAG_FIRST;
    } else if (isTagEqual(newTag, acceptedTagSecond)) {
      return VALID_TAG_SECOND;
    } else {
      return UNKNOWN_TAG;
    }
  }

  return NO_TAG;
}

/**
   Handle the given tag (i.e. play the appropriate audio track).
*/
void handleTag(char theTag[]) {
  if (theTag == UNKNOWN_TAG) {
    Serial.println("## Unknown tag");
    Serial.flush();
  } else if (theTag == VALID_TAG_FIRST) {
    Serial.println("## First tag: Playing audio");
    Serial.flush();

    digitalWrite(PIN_AUDIO_TAG_FIRST, LOW);
    delay(500);
    digitalWrite(PIN_AUDIO_TAG_FIRST, HIGH);
  } else if (theTag == VALID_TAG_SECOND) {
    Serial.println("## Second tag: Playing audio");
    Serial.flush();

    digitalWrite(PIN_AUDIO_TAG_SECOND, LOW);
    delay(500);
    digitalWrite(PIN_AUDIO_TAG_SECOND, HIGH);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_AUDIO_TAG_FIRST, OUTPUT);
  digitalWrite(PIN_AUDIO_TAG_FIRST, HIGH);

  pinMode(PIN_AUDIO_TAG_SECOND, OUTPUT);
  digitalWrite(PIN_AUDIO_TAG_SECOND, HIGH);

  Serial.println(">> Starting Cerebrofono program");
  Serial.flush();
}

void loop() {
  byte mainTag = readMainTag();
  handleTag(mainTag);
}
