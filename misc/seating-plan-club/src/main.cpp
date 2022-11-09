#include <Adafruit_NeoPixel.h>
#include <SerialRFID.h>
#include <SoftwareSerial.h>

const uint8_t RFID_PIN_RX = 2;
const uint8_t RFID_PIN_TX = 3;

SoftwareSerial rfidSoftSerial = SoftwareSerial(RFID_PIN_RX, RFID_PIN_TX);
SerialRFID rfidReader = SerialRFID(rfidSoftSerial);

const int16_t TAG_NOT_FOUND = -1;
const int16_t TAG_UNKNOWN = -2;

const uint8_t NUM_GUESTS = 55;
const uint8_t NUM_TABLES_PER_ROUND = 5;
const uint8_t NUM_ROUNDS = 4;

const uint8_t LED_PER_TABLE = 1;
const uint16_t LED_NUM = LED_PER_TABLE * NUM_ROUNDS * NUM_TABLES_PER_ROUND;
const uint16_t LED_PIN = 4;
const uint8_t LED_BRIGHTNESS = 240;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_RGB + NEO_KHZ800);

const uint8_t EFFECT_NUM_ITERS = 15;
const uint16_t EFFECT_ITER_MS = 200;
const uint32_t EFFECT_COLOR_RANDOM = Adafruit_NeoPixel::Color(150, 150, 150);
const uint32_t EFFECT_COLOR_FINAL = Adafruit_NeoPixel::Color(0, 40, 240);

struct GuestTag
{
  char tagId[SIZE_TAG_ID];
  uint8_t tableIdx[NUM_ROUNDS];
};

GuestTag GUEST_TAGS[NUM_GUESTS] = {
  {"4C009C1CCB07", {1, 1, 4, 1}}, 
  {"4C00A08D2C4D", {3, 1, 3, 3}}, 
  {"4C009C1CC905", {1, 0, 4, 2}}, 
  {"4D00C7FEB7C3", {1, 3, 1, 3}}, 
  {"4D00C80C9A13", {4, 0, 4, 3}}, 
  {"4D009EC65A4F", {1, 3, 4, 4}}, 
  {"4D00A3D01628", {2, 1, 4, 4}}, 
  {"4C009C87FAAD", {2, 3, 4, 2}}, 
  {"4D009E1C529D", {4, 2, 4, 4}}, 
  {"4C009C87F9AE", {4, 1, 4, 2}}, 
  {"4D009DE5CEFB", {3, 2, 3, 2}}, 
  {"4D00A4278A44", {3, 1, 4, 0}}, 
  {"4B00FD8396A3", {3, 0, 4, 1}}, 
  {"4D009EADC4BA", {2, 2, 3, 1}}, 
  {"4B00F445C339", {2, 4, 0, 0}}, 
  {"4C00E2F7A2FB", {2, 4, 3, 2}}, 
  {"4C00E1547F86", {3, 0, 2, 2}}, 
  {"4D009E97F2B6", {0, 2, 0, 4}}, 
  {"4D009DDA979D", {0, 0, 3, 0}}, 
  {"4C0070E8499D", {2, 0, 1, 4}}, 
  {"4D00DBD0C482", {0, 2, 1, 0}}, 
  {"4B00F4543BD0", {3, 2, 1, 4}}, 
  {"4B00F4634D91", {1, 4, 2, 0}}, 
  {"4C0070716825", {0, 4, 2, 3}}, 
  {"4C00E1566893", {1, 0, 0, 0}}, 
  {"3F00FB08BB77", {1, 2, 2, 1}}, 
  {"4C00A11DF505", {4, 0, 3, 4}}, 
  {"4C009CB47C18", {0, 3, 2, 2}}, 
  {"4C00A11DD121", {3, 3, 1, 1}}, 
  {"4C009CB881E9", {0, 0, 1, 3}}, 
  {"4C00356A3122", {3, 2, 0, 0}}, 
  {"4C00370E6510", {2, 0, 0, 3}}, 
  {"4C0035C541FD", {0, 4, 0, 2}}, 
  {"4C0035B572BE", {4, 1, 0, 4}}, 
  {"4C00364BC3F2", {2, 3, 0, 4}}, 
  {"4C003487C23D", {0, 2, 4, 3}}, 
  {"4C00368E8C78", {3, 4, 0, 1}}, 
  {"4C0036B19B50", {0, 0, 0, 1}}, 
  {"4C003716ABC6", {4, 2, 1, 3}}, 
  {"4C00355B4062", {2, 4, 1, 1}}, 
  {"4D00C8278F2D", {0, 1, 1, 1}}, 
  {"4D00C81E22B9", {4, 4, 1, 2}}, 
  {"4D00C7E40D63", {2, 1, 2, 3}}, 
  {"4D00C7DE0450", {4, 3, 2, 0}}, 
  {"4D00C765E807", {0, 1, 2, 4}}, 
  {"4D00C735E15E", {3, 4, 2, 4}}, 
  {"4D00C742A169", {3, 3, 2, 3}}, 
  {"4D00C856E93A", {4, 4, 2, 1}}, 
  {"4D00C81C7AE3", {1, 3, 3, 1}}, 
  {"4D00C7B06258", {2, 3, 3, 0}}, 
  {"4D00C8405E9B", {1, 2, 3, 0}}, 
  {"4D00D0E12559", {4, 4, 0, 3}}, 
  {"4D00C831A713", {4, 3, 3, 2}}, 
  {"4B00F81A943D", {1, 1, 3, 2}}, 
  {"4C00A0D1AD90", {1, 1, 1, 0}}};

    

const uint16_t CLEAR_DELAY_MS = 8000;
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
