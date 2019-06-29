#include <Automaton.h>
#include <Adafruit_NeoPixel.h>

/**
 * Color gamma correction.
 */

const uint8_t PROGMEM gamma8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

uint32_t colorOrange()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[253]),
        pgm_read_byte(&gamma8[106]),
        pgm_read_byte(&gamma8[2]));
}

uint32_t colorPink()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[231]),
        pgm_read_byte(&gamma8[84]),
        pgm_read_byte(&gamma8[128]));
}

uint32_t colorPurple()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[125]),
        pgm_read_byte(&gamma8[60]),
        pgm_read_byte(&gamma8[152]));
}

uint32_t colorYellow()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]));
}

uint32_t colorRed()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[0]));
}

uint32_t colorGreen()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[255]),
        pgm_read_byte(&gamma8[0]));
}

uint32_t colorBlue()
{
    return Adafruit_NeoPixel::Color(
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[0]),
        pgm_read_byte(&gamma8[255]));
}

/**
 * Throughhole LEDs.
 */

const int LED_THROUGH_BRIGHTNESS = 250;
const int LED_THROUGH_NUM = 10;
const int LED_THROUGH_PIN = 12;

Adafruit_NeoPixel ledThrough = Adafruit_NeoPixel(
    LED_THROUGH_NUM,
    LED_THROUGH_PIN,
    NEO_RGB + NEO_KHZ800);

const int LED_THROUGH_COLOR_NUM = 7;

const uint32_t LED_THROUGH_COLORS[LED_THROUGH_COLOR_NUM] = {
    colorBlue(),
    colorRed(),
    colorGreen(),
    colorYellow(),
    colorPink(),
    colorPurple(),
    colorOrange()};

// Blue, Blue, Red, Blue, Red, Green, Orange, Yellow, Red, Green

const int LED_THROUGH_KEY[LED_THROUGH_NUM] = {
    0, 0, 1, 0, 1, 2, 6, 3, 1, 2};

const uint32_t LED_THROUGH_FINAL_COLOR = colorBlue();

Atm_controller ledThroughController;

/**
 * Button.
 */

const int BUTTON_NUM = LED_THROUGH_NUM;

const int BUTTON_PINS[BUTTON_NUM] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

Atm_button buttons[BUTTON_NUM];

/**
 * Reset.
 */

const int RESET_BUTTON_NUM = 3;

const int RESET_BUTTON_IDX[RESET_BUTTON_NUM] = {
    0, 1, 2};

Atm_controller resetController;

/**
 * Program state.
 */

int currLedColorIdx[BUTTON_NUM];

typedef struct programState
{
    int *currLedColorIdx;
    bool isComplete;
} ProgramState;

ProgramState progState = {
    .currLedColorIdx = currLedColorIdx,
    .isComplete = false};

void initState()
{
    progState.isComplete = false;

    for (int i = 0; i < BUTTON_NUM; i++)
    {
        progState.currLedColorIdx[i] = random(0, LED_THROUGH_NUM);
    }
}

/**
 * Throughhole LED functions.
 */

void initLed()
{
    ledThrough.begin();
    ledThrough.setBrightness(LED_THROUGH_BRIGHTNESS);
    ledThrough.clear();
    ledThrough.show();
}

void refreshLed()
{
    uint32_t color;

    ledThrough.clear();

    for (int i = 0; i < LED_THROUGH_NUM; i++)
    {
        color = progState.isComplete
                    ? LED_THROUGH_FINAL_COLOR
                    : LED_THROUGH_COLORS[progState.currLedColorIdx[i]];

        ledThrough.setPixelColor(i, color);
    }

    ledThrough.show();
}

void showSuccessLedPattern()
{
    const int delayIterMs = 30;
    const int valLimit = 250;

    ledThrough.clear();

    for (int k = 0; k < valLimit; k++)
    {
        for (int i = 0; i < LED_THROUGH_NUM; i++)
        {
            ledThrough.setPixelColor(i, k, k, k);
        }

        delay(delayIterMs);

        ledThrough.show();
    }
}

/**
 * Controller functions.
 */

bool isValidCombination()
{
    if (progState.isComplete)
    {
        return true;
    }

    for (int i = 0; i < LED_THROUGH_NUM; i++)
    {
        if (progState.currLedColorIdx[i] != LED_THROUGH_KEY[i])
        {
            return false;
        }
    }

    return true;
}

void onSuccess()
{
    Serial.println(F("Valid LED combination"));
    showSuccessLedPattern();
    progState.isComplete = true;
}

bool isResetPressed()
{
    if (!progState.isComplete)
    {
        return false;
    }

    for (int i = 0; i < RESET_BUTTON_NUM; i++)
    {
        if (buttons[RESET_BUTTON_IDX[i]].state() != Atm_button::PRESSED)
        {
            return false;
        }
    }

    return true;
}

void onReset()
{
    Serial.println(F("Reset"));
    initState();
}

void initControllers()
{
    ledThroughController
        .begin()
        .IF(isValidCombination)
        .onChange(true, onSuccess);

    resetController
        .begin()
        .IF(isResetPressed)
        .onChange(true, onReset);
}

/**
 * Button functions.
 */

void onPress(int idx, int v, int up)
{
    if (progState.isComplete)
    {
        Serial.println(F("Ignoring button"));
        return;
    }

    Serial.print(F("Button :: "));
    Serial.println(idx);

    progState.currLedColorIdx[idx]++;
    progState.currLedColorIdx[idx] = progState.currLedColorIdx[idx] % LED_THROUGH_COLOR_NUM;
}

void initButtons()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        buttons[i]
            .begin(BUTTON_PINS[i])
            .onPress(onPress, i);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initButtons();
    initLed();
    initControllers();

    Serial.println(F(">> Starting button pattern program"));
}

void loop()
{
    automaton.run();
    refreshLed();
}