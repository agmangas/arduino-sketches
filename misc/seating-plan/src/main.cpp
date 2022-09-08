#include <Adafruit_NeoPixel.h>
#include <SerialRFID.h>
#include <SoftwareSerial.h>

const uint8_t PIN_AUDIO_RST = 6;
const uint8_t PIN_AUDIO_ACT = 7;
const uint8_t PIN_AUDIO_TRACK_GUEST = 8;

const uint8_t RFID_PIN_RX = 2;
const uint8_t RFID_PIN_TX = 3;

SoftwareSerial rfidSoftSerial = SoftwareSerial(RFID_PIN_RX, RFID_PIN_TX);
SerialRFID rfidReader = SerialRFID(rfidSoftSerial);

const int16_t TAG_NOT_FOUND = -1;
const int16_t TAG_UNKNOWN = -2;

const uint8_t NUM_GUESTS = 82;
const uint8_t NUM_TABLES = 11;

const uint8_t LED_PER_TABLE = 8;
const uint16_t LED_NUM = LED_PER_TABLE * NUM_TABLES;
const uint16_t LED_PIN = 4;
const uint8_t LED_BRIGHTNESS = 200;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

const uint8_t EFFECT_NUM_ITERS = 15;
const uint16_t EFFECT_ITER_MS = 200;
const uint32_t EFFECT_COLOR_RANDOM = Adafruit_NeoPixel::Color(180, 180, 180);
const uint32_t EFFECT_COLOR_FINAL = Adafruit_NeoPixel::Color(0, 0, 180);

struct GuestTag
{
  char tagId[SIZE_TAG_ID];
  uint8_t tableIdx;
};

GuestTag GUEST_TAGS[NUM_GUESTS] = {
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0},
    {"2B0045230B46", 0}};

char tagBuffer[SIZE_TAG_ID];

bool isTrackPlaying()
{
  return digitalRead(PIN_AUDIO_ACT) == LOW;
}

void playTrack(uint8_t trackPin)
{
  const uint16_t delayPlayMs = 200;

  if (isTrackPlaying())
  {
    Serial.println(F("Skipping: Audio still playing"));
    return;
  }

  Serial.print(F("Playing track on pin: "));
  Serial.println(trackPin);

  digitalWrite(trackPin, LOW);
  pinMode(trackPin, OUTPUT);
  delay(delayPlayMs);
  pinMode(trackPin, INPUT);
}

void initAudioPins()
{
  pinMode(PIN_AUDIO_ACT, INPUT);
  pinMode(PIN_AUDIO_RST, INPUT);
  pinMode(PIN_AUDIO_TRACK_GUEST, INPUT);
}

void resetAudio()
{
  const uint16_t delayResetPinMs = 200;
  const uint16_t delayWaitMs = 2000;

  Serial.println(F("Audio FX reset"));

  digitalWrite(PIN_AUDIO_RST, LOW);
  pinMode(PIN_AUDIO_RST, OUTPUT);
  delay(delayResetPinMs);
  pinMode(PIN_AUDIO_RST, INPUT);

  Serial.println(F("Waiting for Audio FX startup"));

  delay(delayWaitMs);
}

void initLeds()
{
  ledStrip.begin();
  ledStrip.setBrightness(LED_BRIGHTNESS);
  ledStrip.show();
  ledStrip.clear();
}

void showStartupEffect()
{
  const uint16_t delayMsFill = 300;
  const uint16_t delayMsClear = 100;
  const uint16_t numLoops = 5;
  const uint32_t color = Adafruit_NeoPixel::Color(0, 255, 0);

  for (uint16_t i = 0; i < numLoops; i++)
  {
    ledStrip.fill(color);
    ledStrip.show();

    delay(delayMsFill);

    ledStrip.clear();
    ledStrip.show();

    delay(delayMsClear);
  }

  ledStrip.clear();
  ledStrip.show();
}

int16_t readRfid()
{
  bool tagFound = rfidReader.readTag(tagBuffer, sizeof(tagBuffer));

  if (!tagFound)
  {
    return TAG_NOT_FOUND;
  }

  Serial.print(F("Tag: "));
  Serial.println(tagBuffer);

  for (uint8_t idxGuest = 0; idxGuest < NUM_GUESTS; idxGuest++)
  {
    if (SerialRFID::isEqualTag(tagBuffer, GUEST_TAGS[idxGuest].tagId))
    {
      Serial.print(F("Guest: "));
      Serial.println(idxGuest);

      return idxGuest;
    }
  }

  return TAG_UNKNOWN;
}

uint16_t getTableFirstPixel(uint8_t tableIdx)
{
  return tableIdx * LED_PER_TABLE;
}

void showGuest(uint8_t idxGuest)
{
  if (idxGuest >= NUM_GUESTS)
  {
    Serial.println(F("WARN: Unexpected guest index"));
    return;
  }

  uint8_t tableIdx = GUEST_TAGS[idxGuest].tableIdx;

  if (tableIdx >= NUM_TABLES)
  {
    Serial.println(F("WARN: Unexpected table index"));
    return;
  }

  playTrack(PIN_AUDIO_TRACK_GUEST);

  for (uint8_t i = 0; i < EFFECT_NUM_ITERS; i++)
  {
    ledStrip.clear();

    ledStrip.fill(
        EFFECT_COLOR_RANDOM,
        getTableFirstPixel(random(0, NUM_TABLES)),
        LED_PER_TABLE);

    ledStrip.show();

    delay(EFFECT_ITER_MS);
  }

  ledStrip.clear();

  ledStrip.fill(
      EFFECT_COLOR_FINAL,
      getTableFirstPixel(tableIdx),
      LED_PER_TABLE);

  ledStrip.show();
}

void mainLoop()
{
  int16_t idxGuest = readRfid();

  if (idxGuest == TAG_NOT_FOUND || idxGuest == TAG_UNKNOWN || idxGuest < 0)
  {
    return;
  }

  showGuest(idxGuest);
}

void setup()
{
  Serial.begin(9600);

  rfidSoftSerial.begin(9600);
  rfidSoftSerial.listen();

  initLeds();
  initAudioPins();
  resetAudio();

  Serial.println(F(">> Seating plan"));

  showStartupEffect();
}

void loop()
{
  mainLoop();
}