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

const uint16_t TRACK_MARK_MS_ONE = 8000;

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

const uint16_t LED_NUM_BARS = 12;

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

const uint16_t LED_NUM_WHEEL = 180;
const uint8_t LED_PIN_WHEEL = 11;

Adafruit_NeoPixel ledWheel = Adafruit_NeoPixel(
    LED_NUM_WHEEL,
    LED_PIN_WHEEL,
    NEO_GRB + NEO_KHZ800);

/**
 * LED effects timer.
 */

Atm_timer timerLed;

const uint16_t TIMER_LED_MS = 25;
const uint16_t TIMER_MODULO_WHEEL = 1;
const uint16_t TIMER_MODULO_BLINK = 10;

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

const uint32_t COLOR_RED = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(255, 0, 0));

const uint32_t COLOR_GREEN = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(0, 255, 0));

const uint32_t COLOR_ORANGE = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(255, 165, 0));

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

uint16_t moduloBars = 0;

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

void boardPixelBlink(uint32_t color, uint16_t numLoops = 2, unsigned long delayMs = 25)
{
    uint16_t counter = 0;

    while (numLoops == 0 || counter < numLoops) {
        boardPixel.fill(color);
        boardPixel.show();
        delay(delayMs);
        boardPixel.clear();
        boardPixel.show();
        delay(delayMs);
        counter++;
    }
}

void initAudio()
{
    const uint8_t volLeft = 0;
    const uint8_t volRight = 0;

    if (!musicPlayer.begin()) {
        Serial.println("VS1053 not found");
        boardPixelBlink(COLOR_RED, 0, 250);
    }

    Serial.println("VS1053 OK");

    if (!SD.begin(CARDCS)) {
        Serial.println("SD failed or not found");
        boardPixelBlink(COLOR_RED, 0, 250);
    }

    Serial.println("SD OK");
    Serial.println("Listing SD root");

    printDirectory(SD.open("/"), 0);

    // Set volume for left, right channels
    // lower numbers == louder volume
    musicPlayer.setVolume(volLeft, volRight);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
}

void playTrack(String trackName)
{
    if (!musicPlayer.stopped()) {
        Serial.println("Cannot play track: musicPlayer.stopped() != false");
    }

    Serial.print("Playing: ");
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

uint16_t getTimerModuloBars()
{
    if (moduloBars <= 0) {
        uint16_t ticksMarkOne = ceil((double)TRACK_MARK_MS_ONE / (double)TIMER_LED_MS);
        uint16_t ticksPerPixel = ceil((double)ticksMarkOne / (double)LED_NUM_BARS);
        moduloBars = ticksPerPixel;

        Serial.print("Modulo bars: ");
        Serial.println(moduloBars);
    }

    return moduloBars;
}

void ledTick()
{
    uint16_t moduloBars = getTimerModuloBars();

    if (progState.isLedBarsUnlocked) {
        progState.ledTickBars++;

        if (progState.ledTickBars % moduloBars == 0) {
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

uint32_t randomColor()
{
    byte randVal = random(0, 3);

    if (randVal == 0) {
        return Adafruit_NeoPixel::Color(random(50, 250), 0, 0);
    } else if (randVal == 1) {
        return Adafruit_NeoPixel::Color(0, random(50, 250), 0);
    } else {
        return Adafruit_NeoPixel::Color(0, 0, random(50, 250));
    }
}

void refreshLedBars()
{
    if (!progState.isLedBarsUnlocked) {
        for (uint8_t i = 0; i < BARS_NUM; i++) {
            ledsBar[i].clear();
            ledsBar[i].show();
        }

        return;
    }

    if (!progState.isLedWheelUnlocked) {
        uint16_t fillCount;

        for (uint8_t i = 0; i < BARS_NUM; i++) {
            fillCount = progState.ledPivotBars < ledsBar[i].numPixels()
                ? progState.ledPivotBars
                : ledsBar[i].numPixels();

            if (fillCount > 0) {
                ledsBar[i].fill(COLOR_ORANGE, 0, fillCount);
                ledsBar[i].show();
            }
        }

        return;
    }

    if (progState.ledTickWheel % TIMER_MODULO_BLINK == 0) {
        uint32_t color = randomColor();

        for (uint8_t i = 0; i < BARS_NUM; i++) {
            ledsBar[i].fill(color);
            ledsBar[i].show();
        }
    }
}

void refreshLedWheel()
{
    const uint16_t fillCount = 45;

    if (!progState.isLedWheelUnlocked) {
        ledWheel.clear();
        ledWheel.show();
        return;
    }

    uint16_t fillFirst = progState.ledPivotWheel % ledWheel.numPixels();

    ledWheel.clear();
    ledWheel.fill(COLOR_ORANGE, fillFirst, fillCount);
    ledWheel.show();
}

void updateUnlockState()
{
    if (!progState.isLedBarsUnlocked || progState.isLedWheelUnlocked) {
        return;
    }

    if (progState.ledPivotBars >= LED_NUM_BARS) {
        Serial.println("Unlocking wheel LED");
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
    if (progState.isLedBarsUnlocked) {
        Serial.println("Bars LED unlocked already");
        return;
    }

    Serial.println("Unlocking bars LED");
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
    Serial1.begin(9600);

    initState();
    initLeds();
    initLedTimer();
    initButtons();
    initAudio();

    Serial.println(">> Time machine");

    boardPixelBlink(COLOR_GREEN, 10, 50);
}

void loop()
{
    automaton.run();
}
