#include <Adafruit_NeoPixel.h>
#include <SerialRFID.h>
#include <SoftwareSerial.h>

const uint8_t RFID_PIN_RX = 2;
const uint8_t RFID_PIN_TX = 3;

SoftwareSerial rfidSoftSerial = SoftwareSerial(RFID_PIN_RX, RFID_PIN_TX);
SerialRFID rfidReader = SerialRFID(rfidSoftSerial);

const int16_t TAG_NOT_FOUND = -1;
const int16_t TAG_UNKNOWN = -2;

const uint8_t NUM_GUESTS = 50;
const uint8_t NUM_TABLES_PER_ROUND = 5;
const uint8_t NUM_ROUNDS = 4;

const uint8_t LED_PER_TABLE = 7;
const uint16_t LED_NUM = LED_PER_TABLE * NUM_ROUNDS * NUM_TABLES_PER_ROUND;
const uint16_t LED_PIN = 4;
const uint8_t LED_BRIGHTNESS = 240;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

const uint8_t EFFECT_NUM_ITERS = 15;
const uint16_t EFFECT_ITER_MS = 200;
const uint32_t EFFECT_COLOR_RANDOM = Adafruit_NeoPixel::Color(150, 150, 150);
const uint32_t EFFECT_COLOR_FINAL = Adafruit_NeoPixel::Color(0, 240, 0);

struct GuestTag
{
  char tagId[SIZE_TAG_ID];
  uint8_t tableIdx[NUM_ROUNDS];
};

GuestTag GUEST_TAGS[NUM_GUESTS] = {
    {"FFFFFFFFFFFF", {1, 4, 3, 3}},
    {"FFFFFFFFFFFF", {1, 2, 4, 1}},
    {"FFFFFFFFFFFF", {4, 0, 4, 0}},
    {"FFFFFFFFFFFF", {4, 4, 3, 2}},
    {"FFFFFFFFFFFF", {4, 4, 1, 3}},
    {"FFFFFFFFFFFF", {4, 1, 4, 3}},
    {"FFFFFFFFFFFF", {4, 0, 3, 3}},
    {"FFFFFFFFFFFF", {4, 4, 0, 0}},
    {"FFFFFFFFFFFF", {3, 0, 1, 2}},
    {"FFFFFFFFFFFF", {2, 3, 2, 2}},
    {"FFFFFFFFFFFF", {2, 2, 3, 0}},
    {"FFFFFFFFFFFF", {4, 2, 4, 2}},
    {"FFFFFFFFFFFF", {4, 4, 4, 1}},
    {"FFFFFFFFFFFF", {4, 3, 0, 3}},
    {"FFFFFFFFFFFF", {0, 3, 3, 4}},
    {"FFFFFFFFFFFF", {1, 3, 3, 2}},
    {"FFFFFFFFFFFF", {2, 0, 0, 0}},
    {"FFFFFFFFFFFF", {2, 3, 0, 1}},
    {"FFFFFFFFFFFF", {0, 1, 1, 4}},
    {"FFFFFFFFFFFF", {0, 1, 3, 1}},
    {"FFFFFFFFFFFF", {0, 1, 0, 0}},
    {"FFFFFFFFFFFF", {0, 0, 0, 4}},
    {"FFFFFFFFFFFF", {1, 0, 3, 0}},
    {"FFFFFFFFFFFF", {0, 3, 2, 3}},
    {"FFFFFFFFFFFF", {0, 1, 4, 2}},
    {"FFFFFFFFFFFF", {0, 0, 4, 1}},
    {"FFFFFFFFFFFF", {0, 2, 4, 4}},
    {"FFFFFFFFFFFF", {4, 3, 4, 4}},
    {"FFFFFFFFFFFF", {3, 2, 2, 2}},
    {"FFFFFFFFFFFF", {2, 1, 3, 2}},
    {"FFFFFFFFFFFF", {2, 0, 2, 3}},
    {"FFFFFFFFFFFF", {1, 2, 0, 3}},
    {"FFFFFFFFFFFF", {3, 2, 0, 1}},
    {"FFFFFFFFFFFF", {3, 3, 0, 2}},
    {"FFFFFFFFFFFF", {3, 2, 4, 3}},
    {"FFFFFFFFFFFF", {3, 4, 3, 4}},
    {"FFFFFFFFFFFF", {2, 0, 1, 1}},
    {"FFFFFFFFFFFF", {2, 4, 2, 0}},
    {"FFFFFFFFFFFF", {3, 0, 2, 1}},
    {"FFFFFFFFFFFF", {1, 4, 1, 1}},
    {"FFFFFFFFFFFF", {1, 1, 2, 1}},
    {"FFFFFFFFFFFF", {1, 4, 2, 4}},
    {"FFFFFFFFFFFF", {1, 2, 1, 2}},
    {"FFFFFFFFFFFF", {2, 3, 1, 0}},
    {"FFFFFFFFFFFF", {1, 1, 0, 4}},
    {"FFFFFFFFFFFF", {3, 1, 2, 0}},
    {"FFFFFFFFFFFF", {3, 1, 1, 3}},
    {"FFFFFFFFFFFF", {0, 4, 1, 0}},
    {"FFFFFFFFFFFF", {3, 3, 1, 4}},
    {"FFFFFFFFFFFF", {2, 2, 2, 4}}};

const uint16_t CLEAR_DELAY_MS = 5000;
uint32_t lastEventMillis = 0;

char tagBuffer[SIZE_TAG_ID];

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
      Serial.print(idxGuest);

      return idxGuest;
    }
  }

  return TAG_UNKNOWN;
}

uint16_t getTableFirstPixel(uint8_t tableIdx, uint8_t roundIdx)
{
  return (roundIdx * LED_PER_TABLE * NUM_TABLES_PER_ROUND) + tableIdx * LED_PER_TABLE;
}

void showGuest(uint8_t idxGuest)
{
  if (idxGuest >= NUM_GUESTS)
  {
    Serial.println(F("WARN: Unexpected guest index"));
    return;
  }

  Serial.println(F("LED effect: start"));

  for (uint8_t i = 0; i < EFFECT_NUM_ITERS; i++)
  {
    ledStrip.clear();

    ledStrip.fill(
        EFFECT_COLOR_RANDOM,
        getTableFirstPixel(random(0, NUM_TABLES_PER_ROUND), random(0, NUM_ROUNDS)),
        LED_PER_TABLE);

    ledStrip.show();

    delay(EFFECT_ITER_MS);
  }

  Serial.println(F("LED effect: end"));

  ledStrip.clear();

  for (uint8_t idxRound = 0; idxRound < NUM_ROUNDS; idxRound++)
  {
    ledStrip.fill(
        EFFECT_COLOR_FINAL,
        getTableFirstPixel(GUEST_TAGS[idxGuest].tableIdx[idxRound], idxRound),
        LED_PER_TABLE);
  }

  ledStrip.show();

  lastEventMillis = millis();
}

void mainLoop()
{
  uint32_t diffLast = millis() - lastEventMillis;

  if (lastEventMillis > 0 && diffLast >= CLEAR_DELAY_MS)
  {
    Serial.println(F("Clearing LED"));
    lastEventMillis = 0;
    ledStrip.clear();
    ledStrip.show();
  }

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

  showStartupEffect();

  Serial.println(F(">> Seating plan (club)"));
}

void loop()
{
  mainLoop();
}