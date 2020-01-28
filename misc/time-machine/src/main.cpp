#include <Adafruit_NeoPixel.h>
#include <Adafruit_VS1053.h>
#include <Automaton.h>
#include <SD.h>
#include <SPI.h>

#define VS1053_RESET -1 // VS1053 reset pin (not used)
#define VS1053_CS 6 // VS1053 chip select pin (output)
#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define CARDCS 5 // Card chip select pin
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin

/**
 * Audio player instance.
 */

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(
    VS1053_RESET,
    VS1053_CS,
    VS1053_DCS,
    VS1053_DREQ,
    CARDCS);

/**
 * Audio track names. 
 */

const String TRACK_LED_EFFECT = String("/effect.mp3");

/**
 * Number of wheel bars.
 */

const uint8_t BARS_NUM = 3;

/**
 * Default LED brightness
 */

const uint8_t LED_BRIGHTNESS = 180;

/**
 * Wheel bar LED strips.
 */

const uint16_t LED_NUM_BARS = 20;

const uint8_t LED_NUM_PINS[BARS_NUM] = {
    A2, A3, A4
};

Adafruit_NeoPixel ledsBar[BARS_NUM] = {
    Adafruit_NeoPixel(LED_NUM_BARS, LED_NUM_PINS[0], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_NUM_BARS, LED_NUM_PINS[1], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_NUM_BARS, LED_NUM_PINS[2], NEO_GRB + NEO_KHZ800)
};

/**
 * Wheel LED strip.
 */

const uint16_t LED_NUM_WHEEL = 120;
const uint8_t LED_PIN_WHEEL = 11;

Adafruit_NeoPixel ledWheel = Adafruit_NeoPixel(
    LED_NUM_WHEEL,
    LED_PIN_WHEEL,
    NEO_GRB + NEO_KHZ800);

/**
 * LED effects timer.
 */

Atm_timer timerLed;

const uint16_t TIMER_LED_MS = 20;
const uint16_t TIMER_MODULO_BARS = 2;
const uint16_t TIMER_MODULO_WHEEL = 1;

/**
 * LED unlock button.
 */

const uint8_t BUTTON_UNLOCK_PIN = 12;

Atm_button buttonUnlock;

/**
 * Built-in Neopixel.
 */

const uint16_t LED_NUM_BOARD = 1;
const uint8_t LED_PIN_BOARD = 8;

Adafruit_NeoPixel boardPixel = Adafruit_NeoPixel(
    LED_NUM_BOARD,
    LED_PIN_BOARD,
    NEO_GRB + NEO_KHZ800);

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

void printDirectory(File dir, int numTabs)
{
    while (true) {
        File entry = dir.openNextFile();

        if (!entry) {
            // No more files
            break;
        }

        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }

        Serial.print(entry.name());

        if (entry.isDirectory()) {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            // Files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }

        entry.close();
    }
}

void initAudio()
{
    const uint8_t volLeft = 0;
    const uint8_t volRight = 0;

    if (!musicPlayer.begin()) {
        Serial.println(F("VS1053 not found"));
        while (true) {
            delay(1);
        }
    }

    Serial.println(F("VS1053 OK"));

    if (!SD.begin(CARDCS)) {
        Serial.println(F("SD failer or not found"));
        while (true) {
            delay(1);
        }
    }

    Serial.println(F("SD OK"));
    Serial.println(F("Listing SD root"));

    printDirectory(SD.open("/"), 0);

    // Set volume for left, right channels
    // lower numbers == louder volume
    musicPlayer.setVolume(volLeft, volRight);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
}

void playTrack(String trackName)
{
    if (!musicPlayer.stopped()) {
        Serial.println(F("Cannot play track: musicPlayer.stopped() != false"));
    }

    Serial.print(F("Playing: "));
    Serial.println(trackName);

    musicPlayer.startPlayingFile(trackName.c_str());
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

    boardPixel.begin();
    boardPixel.setBrightness(LED_BRIGHTNESS);
    boardPixel.clear();
    boardPixel.show();
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

void updateUnlockState()
{
    if (!progState.isLedBarsUnlocked || progState.isLedWheelUnlocked) {
        return;
    }

    uint16_t numPixelsBars = 0;

    for (uint8_t i = 0; i < LED_NUM_BARS; i++) {
        numPixelsBars = ledsBar[i].numPixels() > numPixelsBars
            ? ledsBar[i].numPixels()
            : numPixelsBars;
    }

    if (progState.ledPivotBars >= numPixelsBars) {
        Serial.println(F("Unlocking wheel LED"));
        progState.isLedWheelUnlocked = true;
    }
}

void onLedTimer(int idx, int v, int up)
{
    refreshLedBars();
    refreshLedWheel();
    updateUnlockState();
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

void onPressUnlock(int idx, int v, int up)
{
    Serial.println(F("Unlocking bars LED"));
    progState.isLedBarsUnlocked = true;
    playTrack(TRACK_LED_EFFECT);
}

void initButtons()
{
    buttonUnlock
        .begin(BUTTON_UNLOCK_PIN)
        .onPress(onPressUnlock);
}

void setup()
{
    Serial.begin(9600);

    initState();
    initLeds();
    initLedTimer();
    initButtons();
    initAudio();

    Serial.println(F(">> Time machine"));

    boardPixel.fill(Adafruit_NeoPixel::Color(0, 255, 0));
    boardPixel.show();
}

void loop()
{
    automaton.run();
}