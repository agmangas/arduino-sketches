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

const uint8_t NUM_GUESTS = 90;
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
    {"3C00D54B51F3", 0},
    {"3C00D611E61D", 0},
    {"0C007CE39605", 1},
    {"3C00D5D4A29F", 1},
    {"3C00D611A55E", 1},
    {"0C007CE77FE8", 1},
    {"3C00D555EB57", 1},
    {"3C00D52B2EEC", 1},
    {"3C00D64316BF", 1},
    {"3C00D5522992", 2},
    {"3C00D54568C4", 2},
    {"0A00539F36F0", 2},
    {"3C00D55DB501", 2},
    {"3C00D55DAD19", 2},
    {"3C00D5A88DCC", 2},
    {"3C00D52A1BD8", 2},
    {"3C00D535578B", 2},
    {"0A00539F0EC8", 2},
    {"3C00D5F94A5A", 2},
    {"0B00298852F8", 2},
    {"0A005A9D2BE6", 3},
    {"3C0088475EAD", 3},
    {"0C007D78030A", 3},
    {"3C00D54F74D2", 3},
    {"0B002A2EABA4", 3},
    {"0B0027DCE515", 3},
    {"0B00275FDBA8", 3},
    {"3C0088D7E88B", 4},
    {"3C00D5670688", 4},
    {"0C007DD69C3B", 4},
    {"0C007D7BDDD7", 4},
    {"3C00D6260EC2", 4},
    {"3C00D5499F3F", 4},
    {"0C007D08730A", 4},
    {"0C007D8ABA41", 4},
    {"3C00D562C44F", 4},
    {"3C0087F1CE84", 4},
    {"0A0055D0C847", 4},
    {"3C00D5D55B67", 4},
    {"0C007DCB57ED", 4},
    {"0A00530A297A", 5},
    {"3C00D60720CD", 5},
    {"0B002954ADDB", 5},
    {"0B0029535120", 5},
    {"0B00296184C7", 5},
    {"3C00D5A30349", 5},
    {"3C00D650B70D", 5},
    {"0B00292EF5F9", 5},
    {"3C00D562A823", 5},
    {"3C00D52D9B5F", 6},
    {"0C007E2A2179", 6},
    {"3D00D64301A9", 6},
    {"3C00D658B406", 6},
    {"3C00D5749B06", 6},
    {"3C00D595106C", 6},
    {"0C007DBDAA66", 6},
    {"0B0027CF4AA9", 6},
    {"0B00269158E4", 6},
    {"3C00890613A0", 7},
    {"3C0088DFF992", 7},
    {"3C008900F540", 7},
    {"0C007E25DD8A", 7},
    {"3C00891DE048", 7},
    {"3C008919B01C", 7},
    {"3C0087F683CE", 7},
    {"3C00D5A00C45", 8},
    {"3C00D55F7DCB", 8},
    {"0C007DF0B938", 8},
    {"0A0059E97EC4", 8},
    {"0C007D450337", 8},
    {"0C007D5F5E70", 8},
    {"3C00D653F24B", 8},
    {"3C0087F2551C", 8},
    {"0C007D1EA5CA", 9},
    {"3C00D541B31B", 9},
    {"3C00D530B069", 9},
    {"3C00D5AD5713", 9},
    {"3C00D637ED30", 9},
    {"3C00D561E56D", 9},
    {"3C00D5274987", 9},
    {"0A0059EBEC54", 9},
    {"0C007D9FBC52", 10},
    {"3C00D5AA4605", 10},
    {"0C007D49BE86", 10},
    {"3C00D5E98F8F", 10},
    {"0C007CD203A1", 10},
    {"3C00D5C94060", 10},
    {"0C007DC1CF7F", 10},
    {"0C007D43E7D5", 10},
    {"0C007DF71D9B", 10}};

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
      Serial.print(idxGuest);
      Serial.print(F(" Table: "));
      Serial.println(GUEST_TAGS[idxGuest].tableIdx);

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

  Serial.println(F("LED effect: start"));

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

  Serial.println(F("LED effect: end"));

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