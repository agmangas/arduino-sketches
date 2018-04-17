// Constants that enumerate the possible results when polling the RFID reader
// Track 0: "Cinco de la tarde"
// Track 1: "Rioturbio"
// Track 2: "Reina"
// Track 3: "Cristine"
#define NO_TAG 1
#define UNKNOWN_TAG 2
#define VALID_TAG_TRACK_0 3
#define VALID_TAG_TRACK_1 4
#define VALID_TAG_TRACK_2 5
#define VALID_TAG_TRACK_3 6

// Each tag is 16 bytes but the last four are to be discarded.
const int TAG_LEN = 16;
const int ID_LEN = 13;
const int ID_PRINTABLE_LEN = 12;

// IDs of valid tags
char tagTrack0[ID_LEN] = "5C00CADBC28F";
char tagTrack1[ID_LEN] = "5C00CADBA1EC";
char tagTrack2[ID_LEN] = "5C00CB0D20BA";
char tagTrack3[ID_LEN] = "570046666116";

// Digital pins that are connected to the Audio FX board
const byte PIN_AUDIO_TRACK_0 = 8;
const byte PIN_AUDIO_TRACK_1 = 9;
const byte PIN_AUDIO_TRACK_2 = 10;
const byte PIN_AUDIO_TRACK_3 = 11;

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

    if (isTagEqual(newTag, tagTrack0)) {
      return VALID_TAG_TRACK_0;
    } else if (isTagEqual(newTag, tagTrack1)) {
      return VALID_TAG_TRACK_1;
    } else if (isTagEqual(newTag, tagTrack2)) {
      return VALID_TAG_TRACK_2;
    } else if (isTagEqual(newTag, tagTrack3)) {
      return VALID_TAG_TRACK_3;
    } else {
      return UNKNOWN_TAG;
    }
  }

  return NO_TAG;
}

/**
   Plays the audio track connected to the given pin.
*/
void playTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

/**
   Handle the given tag (i.e. play the appropriate audio track).
*/
void handleTag(char theTag[]) {
  if (theTag == UNKNOWN_TAG) {
    Serial.println("## Unknown tag");
  } else if (theTag == VALID_TAG_TRACK_0) {
    Serial.println("## Playing Track 0");
    playTrack(PIN_AUDIO_TRACK_0);
  } else if (theTag == VALID_TAG_TRACK_1) {
    Serial.println("## Playing Track 1");
    playTrack(PIN_AUDIO_TRACK_1);
  } else if (theTag == VALID_TAG_TRACK_2) {
    Serial.println("## Playing Track 2");
    playTrack(PIN_AUDIO_TRACK_2);
  } else if (theTag == VALID_TAG_TRACK_3) {
    Serial.println("## Playing Track 3");
    playTrack(PIN_AUDIO_TRACK_3);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_AUDIO_TRACK_0, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_0, HIGH);

  pinMode(PIN_AUDIO_TRACK_1, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_1, HIGH);

  pinMode(PIN_AUDIO_TRACK_2, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_2, HIGH);

  pinMode(PIN_AUDIO_TRACK_3, OUTPUT);
  digitalWrite(PIN_AUDIO_TRACK_3, HIGH);

  Serial.println(">> Starting Cerebrofono program");
  Serial.flush();
}

void loop() {
  byte mainTag = readMainTag();
  handleTag(mainTag);
}
