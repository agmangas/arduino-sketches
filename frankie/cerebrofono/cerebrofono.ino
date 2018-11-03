#include <Adafruit_NeoPixel.h>

// Constants that enumerate the possible results when polling the RFID reader
// Track 0: "Cinco de la tarde"
// Track 1: "Rioturbio"
// Track 2: "Reina"
// Track 3: "Cristine"
// Track 4: "Profesor"
#define NO_TAG 1
#define UNKNOWN_TAG 2
#define VALID_TAG_TRACK_0 3
#define VALID_TAG_TRACK_1 4
#define VALID_TAG_TRACK_2 5
#define VALID_TAG_TRACK_3 6
#define VALID_TAG_TRACK_4 7

// Each tag is 16 bytes but the last four are to be discarded.
const int TAG_LEN = 16;
const int ID_LEN = 13;
const int ID_PRINTABLE_LEN = 12;

// IDs of valid tags
char tagTrack0[ID_LEN] = "5C00CADBA1EC";
char tagTrack1[ID_LEN] = "5C00CADBC28F";
char tagTrack2[ID_LEN] = "5C00CB0D20BA";
char tagTrack3[ID_LEN] = "570046666116";
char tagTrack4[ID_LEN] = "5C00CADB5A17";

// Digital pins that are connected to the Audio FX board
const byte PIN_AUDIO_TRACK_0 = 8;
const byte PIN_AUDIO_TRACK_1 = 9;
const byte PIN_AUDIO_TRACK_2 = 10;
const byte PIN_AUDIO_TRACK_3 = 11;
const byte PIN_AUDIO_TRACK_4 = 12;

// Non-printable tag buffer characters
const int TAG_CHAR_STX = 2;
const int TAG_CHAR_CR = 13;
const int TAG_CHAR_LF = 10;
const int TAG_CHAR_ETX = 3;

// Time (ms) that the equalizer-like LED effect should last
const int LED_MS_DEFAULT = 6000;
const int LED_MS_TRACK_0 = 7500;
const int LED_MS_TRACK_1 = 10500;
const int LED_MS_TRACK_2 = 10500;
const int LED_MS_TRACK_3 = 11500;
const int LED_MS_TRACK_4 = 29500;
const int LED_EFFECT_STEP_MS = 3;

// NeoPixels PIN and total number
const uint16_t NEOPIXEL_NUM = 60;
const uint8_t NEOPIXEL_PIN = 4;

// Initialize the NeoPixel instances
Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Audio track colors
const uint32_t COLOR_DEFAULT = pixelStrip.Color(126, 32, 198);
const uint32_t COLOR_TRACK_0 = pixelStrip.Color(0, 255, 0);
const uint32_t COLOR_TRACK_1 = pixelStrip.Color(255, 0, 0);
const uint32_t COLOR_TRACK_2 = pixelStrip.Color(231, 190, 0);
const uint32_t COLOR_TRACK_3 = pixelStrip.Color(60, 72, 231);
const uint32_t COLOR_TRACK_4 = pixelStrip.Color(255, 255, 255);

/**
   Returns the color for the given tag / track.
*/
uint32_t getTrackColor(byte theTag) {
  switch (theTag) {
    case VALID_TAG_TRACK_0:
      return COLOR_TRACK_0;
    case VALID_TAG_TRACK_1:
      return COLOR_TRACK_1;
    case VALID_TAG_TRACK_2:
      return COLOR_TRACK_2;
    case VALID_TAG_TRACK_3:
      return COLOR_TRACK_3;
    case VALID_TAG_TRACK_4:
      return COLOR_TRACK_4;
    default:
      return COLOR_DEFAULT;
  }
}

/**
   Returns the duration (ms) for the given tag / track.
*/
unsigned long getTrackTimeMs(byte theTag) {
  switch (theTag) {
    case VALID_TAG_TRACK_0:
      return LED_MS_TRACK_0;
    case VALID_TAG_TRACK_1:
      return LED_MS_TRACK_1;
    case VALID_TAG_TRACK_2:
      return LED_MS_TRACK_2;
    case VALID_TAG_TRACK_3:
      return LED_MS_TRACK_3;
    case VALID_TAG_TRACK_4:
      return LED_MS_TRACK_4;
    default:
      return LED_MS_DEFAULT;
  }
}

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
    } else if (isTagEqual(newTag, tagTrack4)) {
      return VALID_TAG_TRACK_4;
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
  Serial.print("Setting pin to LOW / OUTPUT: ");
  Serial.println(trackPin);

  digitalWrite(trackPin, LOW);
  pinMode(trackPin, OUTPUT);
  delay(200);

  Serial.println("Going back to INPUT");

  pinMode(trackPin, INPUT);
  delay(200);
}

/**
   Handle the given tag (i.e. play the appropriate audio track).
*/
void handleTag(byte theTag) {
  switch (theTag) {
    case VALID_TAG_TRACK_0:
      Serial.println("## Playing Track 0");
      playTrack(PIN_AUDIO_TRACK_0);
      displayAudioLedEffect(theTag);
      break;
    case VALID_TAG_TRACK_1:
      Serial.println("## Playing Track 1");
      playTrack(PIN_AUDIO_TRACK_1);
      displayAudioLedEffect(theTag);
      break;
    case VALID_TAG_TRACK_2:
      Serial.println("## Playing Track 2");
      playTrack(PIN_AUDIO_TRACK_2);
      displayAudioLedEffect(theTag);
      break;
    case VALID_TAG_TRACK_3:
      Serial.println("## Playing Track 3");
      playTrack(PIN_AUDIO_TRACK_3);
      displayAudioLedEffect(theTag);
      break;
    case VALID_TAG_TRACK_4:
      Serial.println("## Playing Track 4");
      playTrack(PIN_AUDIO_TRACK_4);
      displayAudioLedEffect(theTag);
      break;
    case UNKNOWN_TAG:
      Serial.println("## Unknown tag");
      break;
  }
}

/**
   Display an audio equalizer-like LED effect.
*/
void displayAudioLedEffect(byte theTag) {
  int limitLo = floor(NEOPIXEL_NUM * 0.45);
  int limitHi = floor(NEOPIXEL_NUM * 1.00);

  uint32_t trackColor = getTrackColor(theTag);
  unsigned long trackMs = getTrackTimeMs(theTag);

  unsigned long now;
  unsigned long diff;
  unsigned long ini = millis();
  bool keepGoing = true;

  while (keepGoing) {
    int currTarget = random(limitLo, limitHi);
    int currLevel = 0;

    clearPixels();

    for (int i = 0; i < currTarget; i++) {
      pixelStrip.setPixelColor(i, trackColor);
      pixelStrip.show();
      delay(LED_EFFECT_STEP_MS);
    }

    for (int i = (currTarget - 1); i >= 0; i--) {
      pixelStrip.setPixelColor(i, 0, 0, 0);
      pixelStrip.show();
      delay(LED_EFFECT_STEP_MS);
    }

    now = millis();

    if (now < ini) {
      keepGoing = false;
    } else {
      diff = now - ini;
      if (diff > trackMs) keepGoing = false;
    }
  }

  clearPixels();
}

/**
   Turns the LED strip off.
*/
void clearPixels() {
  pixelStrip.clear();
  pixelStrip.show();
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_AUDIO_TRACK_0, INPUT);
  pinMode(PIN_AUDIO_TRACK_1, INPUT);
  pinMode(PIN_AUDIO_TRACK_2, INPUT);
  pinMode(PIN_AUDIO_TRACK_3, INPUT);
  pinMode(PIN_AUDIO_TRACK_4, INPUT);

  pixelStrip.begin();
  pixelStrip.setBrightness(200);
  pixelStrip.show();

  clearPixels();

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixelStrip.setPixelColor(i, 200, 200, 200);
    pixelStrip.show();
    delay(5);
  }

  clearPixels();

  Serial.println(">> Starting Cerebrofono program");
  Serial.flush();
}

void loop() {
  byte mainTag = readMainTag();
  handleTag(mainTag);
}
