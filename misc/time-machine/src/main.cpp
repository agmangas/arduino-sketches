#include <Adafruit_NeoPixel.h>
#include <Automaton.h>

const uint8_t BARS_NUM = 4;
const uint8_t LED_BRIGHTNESS = 200;
const uint16_t LED_NUM_BARS = 20;
const uint16_t LED_NUM_WHEEL = 120;

Adafruit_NeoPixel ledsBar[BARS_NUM] = {
    Adafruit_NeoPixel(LED_NUM_BARS, 2, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_NUM_BARS, 3, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_NUM_BARS, 4, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_NUM_BARS, 5, NEO_GRB + NEO_KHZ800)
};

Adafruit_NeoPixel ledWheel = Adafruit_NeoPixel(
    LED_NUM_WHEEL, 6, NEO_GRB + NEO_KHZ800);

Atm_timer timerLed;

const uint16_t TIMER_LED_MS = 20;
const uint16_t TIMER_MODULO_BARS = 2;
const uint16_t TIMER_MODULO_WHEEL = 1;

typedef struct programState {
    bool isLedBarsUnlocked;
    bool isLedWheelUnlocked;
    uint32_t ledTickBars;
    uint32_t ledTickWheel;
    uint32_t ledPivotBars;
    uint32_t ledPivotWheel;
} ProgramState;

ProgramState progState;

void initState()
{
    progState.isLedBarsUnlocked = false;
    progState.isLedWheelUnlocked = false;
    progState.ledTickBars = 0;
    progState.ledTickWheel = 0;
    progState.ledPivotBars = 0;
    progState.ledPivotWheel = 0;
}

void initLeds()
{
    for (int i = 0; i < BARS_NUM; i++) {
        ledsBar[i].begin();
        ledsBar[i].setBrightness(LED_BRIGHTNESS);
        ledsBar[i].clear();
        ledsBar[i].show();
    }

    ledWheel.begin();
    ledWheel.setBrightness(LED_BRIGHTNESS);
    ledWheel.clear();
    ledWheel.show();
}

void ledTick()
{
    if (progState.isLedBarsUnlocked) {
        progState.ledTickBars++;

        if (progState.ledTickBars % TIMER_MODULO_BARS == 0) {
            progState.ledPivotBars++;
        }
    }

    if (progState.isLedWheelUnlocked) {
        progState.ledTickWheel++;

        if (progState.ledTickWheel % TIMER_MODULO_WHEEL == 0) {
            progState.ledPivotWheel++;
        }
    }
}

void refreshLedBars()
{
    const uint32_t color = Adafruit_NeoPixel::Color(0, 0, 255);

    if (!progState.isLedBarsUnlocked) {
        for (uint8_t i = 0; i < LED_NUM_BARS; i++) {
            ledsBar[i].clear();
            ledsBar[i].show();
        }

        return;
    }

    uint16_t fillCount;

    for (uint8_t i = 0; i < LED_NUM_BARS; i++) {
        fillCount = progState.ledPivotBars < ledsBar[i].numPixels()
            ? progState.ledPivotBars
            : ledsBar[i].numPixels();

        if (fillCount > 0) {
            ledsBar[i].fill(color, 0, fillCount);
            ledsBar[i].show();
        }
    }
}

void refreshLedWheel()
{
    const uint32_t color = Adafruit_NeoPixel::Color(0, 0, 255);
    const uint16_t fillCount = 5;

    if (!progState.isLedWheelUnlocked) {
        ledWheel.clear();
        ledWheel.show();
        return;
    }

    uint16_t fillFirst = progState.ledPivotWheel % ledWheel.numPixels();

    ledWheel.clear();
    ledWheel.fill(color, fillFirst, fillCount);
    ledWheel.show();
}

void onLedTimer(int idx, int v, int up)
{
    refreshLedBars();
    refreshLedWheel();
    ledTick();
}

void initLedTimer()
{
    timerLed
        .begin(TIMER_LED_MS)
        .repeat(-1)
        .onTimer(onLedTimer)
        .start();
}

void setup()
{
    Serial.begin(9600);

    initState();
    initLeds();
    initLedTimer();

    Serial.println(F(">> Time machine"));
}

void loop()
{
    automaton.run();
}