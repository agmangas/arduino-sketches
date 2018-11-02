#include <SerialRFID.h>
#include <Adafruit_NeoPixel.h>

/**
   Audio track constants
   Track 0: "Cinco de la tarde"
   Track 1: "Rioturbio"
   Track 2: "Reina"
   Track 3: "Cristine"
   Track 4: "Profesor"
   Track 5: "Mono"
*/

const byte NUM_TRACKS = 5;

const char TRACK_TAGS[NUM_TRACKS][SIZE_TAG_ID] = {
  "5C00CADBA1EC",
  "5C00CADBC28F",
  "5C00CB0D20BA",
  "570046666116",
  "5C00CADB5A17"
};

// "Mono" was on pin 13

const byte TRACK_PINS[NUM_TRACKS] = {
  8,
  9,
  10,
  11,
  12
};

const unsigned long TRACK_LENGTH_MS[NUM_TRACKS] = {
  7500,
  10500,
  10500,
  11500,
  29500
};

const uint32_t TRACK_COLORS[NUM_TRACKS] = {
  Adafruit_NeoPixel::Color(0, 255, 0),
  Adafruit_NeoPixel::Color(255, 0, 0),
  Adafruit_NeoPixel::Color(231, 190, 0),
  Adafruit_NeoPixel::Color(60, 72, 231),
  Adafruit_NeoPixel::Color(255, 255, 255)
};

/**
   RFID reader
*/

SerialRFID rfid(Serial);

char tagId[SIZE_TAG_ID];

/**
   LEDs
*/

const byte DEFAULT_BRIGHTNESS = 160;

const int LED_EFFECT_STEP_MS = 3;

const uint16_t NEOPIXEL_NUM = 60;
const uint8_t NEOPIXEL_PIN = 4;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(
                                 NEOPIXEL_NUM,
                                 NEOPIXEL_PIN,
                                 NEO_GRB + NEO_KHZ800);

/**
   Audio FX functions.
*/

void initAudio() {
  for (int i = 0; i < NUM_TRACKS; i++) {
    digitalWrite(TRACK_PINS[i], HIGH);
    pinMode(TRACK_PINS[i], OUTPUT);
  }
}

void playTrack(byte trackPin) {
  digitalWrite(trackPin, LOW);
  delay(500);
  digitalWrite(trackPin, HIGH);
}

/**
   LED strip functions.
*/

void initLeds() {
  pixelStrip.begin();
  pixelStrip.setBrightness(DEFAULT_BRIGHTNESS);
  pixelStrip.show();
  clearLeds();
}

void clearLeds() {
  pixelStrip.clear();
  pixelStrip.show();
}

void displayAudioLedEffect(int idx) {
  int limitLo = floor(NEOPIXEL_NUM * 0.45);
  int limitHi = floor(NEOPIXEL_NUM * 1.00);

  uint32_t trackColor = TRACK_COLORS[idx];
  unsigned long trackMs = TRACK_LENGTH_MS[idx];

  unsigned long now;
  unsigned long diff;
  unsigned long ini = millis();
  bool keepGoing = true;

  while (keepGoing) {
    int currTarget = random(limitLo, limitHi);
    int currLevel = 0;

    clearLeds();

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

  clearLeds();
}

/**
   RFID reader functions.
*/

void onTrackTag(int idx) {
  playTrack(TRACK_PINS[idx]);
  displayAudioLedEffect(idx);
}

void readRfid() {
  if (rfid.readTag(tagId, sizeof(tagId))) {
    Serial.print("Tag: ");
    Serial.print(tagId);

    for (int i = 0; i < NUM_TRACKS; i++) {
      if (SerialRFID::isEqualTag(tagId, TRACK_TAGS[i])) {
        Serial.print("Match for track: ");
        Serial.println(i);
        onTrackTag(i);
        return;
      }
    }
  }
}

/**
   Entrypoint.
*/

void setup() {
  Serial.begin(9600);

  initAudio();
  initLeds();

  Serial.println(">> Starting Cerebrofono program");
  Serial.flush();
}

void loop() {
  readRfid();
  clearLeds();
}
