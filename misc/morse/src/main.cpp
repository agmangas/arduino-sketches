#include <Automaton.h>
#include <CircularBuffer.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

/**
 * LCD display.
 */

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

Atm_timer lcdTimer;
const int LCD_TIMER_MS = 500;

const String STR_DEFAULT = String("Enter morse code");
const String STR_SUCCESS = String("Access granted");
const String STR_KEY = String("nevaria");

/**
 * Morse buffer.
 */

typedef struct morseItem {
    unsigned long time;
    byte val;
} MorseItem;

const byte MORSE_DOT = 0;
const byte MORSE_DASH = 1;

const int MORSE_LETTER_TIMEOUT_MS = 1000;
const int MORSE_BUF_SIZE = 100;

CircularBuffer<MorseItem, MORSE_BUF_SIZE> morseBuf;

/**
 * Program state.
 */

String strMorseDecoded = String("");
bool isComplete = false;
bool isTouched = false;

/**
 * Audio FX.
 */

const byte PIN_AUDIO_RST = 4;
const byte PIN_AUDIO_ACT = 5;
const byte PIN_TRACK_SUCCESS_ONE = 6;
const byte PIN_TRACK_SUCCESS_TWO = 7;

/**
 * Morse dict. 
 */

byte MORSE_A[] = { MORSE_DOT, MORSE_DASH };
byte MORSE_B[] = { MORSE_DASH, MORSE_DOT, MORSE_DOT, MORSE_DOT };
byte MORSE_C[] = { MORSE_DASH, MORSE_DOT, MORSE_DASH, MORSE_DOT };
byte MORSE_D[] = { MORSE_DASH, MORSE_DOT, MORSE_DOT };
byte MORSE_E[] = { MORSE_DOT };
byte MORSE_F[] = { MORSE_DOT, MORSE_DOT, MORSE_DASH, MORSE_DOT };
byte MORSE_G[] = { MORSE_DASH, MORSE_DASH, MORSE_DOT };
byte MORSE_H[] = { MORSE_DOT, MORSE_DOT, MORSE_DOT, MORSE_DOT };
byte MORSE_I[] = { MORSE_DOT, MORSE_DOT };
byte MORSE_J[] = { MORSE_DOT, MORSE_DASH, MORSE_DASH, MORSE_DASH };
byte MORSE_K[] = { MORSE_DASH, MORSE_DOT, MORSE_DASH };
byte MORSE_L[] = { MORSE_DOT, MORSE_DASH, MORSE_DOT, MORSE_DOT };
byte MORSE_M[] = { MORSE_DASH, MORSE_DASH };
byte MORSE_N[] = { MORSE_DASH, MORSE_DOT };
byte MORSE_O[] = { MORSE_DASH, MORSE_DASH, MORSE_DASH };
byte MORSE_P[] = { MORSE_DOT, MORSE_DASH, MORSE_DASH, MORSE_DOT };
byte MORSE_Q[] = { MORSE_DASH, MORSE_DASH, MORSE_DOT, MORSE_DASH };
byte MORSE_R[] = { MORSE_DOT, MORSE_DASH, MORSE_DOT };
byte MORSE_S[] = { MORSE_DOT, MORSE_DOT, MORSE_DOT };
byte MORSE_T[] = { MORSE_DASH };
byte MORSE_U[] = { MORSE_DOT, MORSE_DOT, MORSE_DASH };
byte MORSE_V[] = { MORSE_DOT, MORSE_DOT, MORSE_DOT, MORSE_DASH };
byte MORSE_W[] = { MORSE_DOT, MORSE_DASH, MORSE_DASH };
byte MORSE_X[] = { MORSE_DASH, MORSE_DOT, MORSE_DOT, MORSE_DASH };
byte MORSE_Y[] = { MORSE_DASH, MORSE_DOT, MORSE_DASH, MORSE_DASH };
byte MORSE_Z[] = { MORSE_DASH, MORSE_DASH, MORSE_DOT, MORSE_DOT };

typedef struct morseDictEntry {
    char val;
    byte* def;
    size_t size;
} MorseDictEntry;

const int MORSE_DICT_NUM = 26;

MorseDictEntry morseDict[MORSE_DICT_NUM] = {
    { 'a', MORSE_A, sizeof(MORSE_A) },
    { 'b', MORSE_B, sizeof(MORSE_B) },
    { 'c', MORSE_C, sizeof(MORSE_C) },
    { 'd', MORSE_D, sizeof(MORSE_D) },
    { 'e', MORSE_E, sizeof(MORSE_E) },
    { 'f', MORSE_F, sizeof(MORSE_F) },
    { 'g', MORSE_G, sizeof(MORSE_G) },
    { 'h', MORSE_H, sizeof(MORSE_H) },
    { 'i', MORSE_I, sizeof(MORSE_I) },
    { 'j', MORSE_J, sizeof(MORSE_J) },
    { 'k', MORSE_K, sizeof(MORSE_K) },
    { 'l', MORSE_L, sizeof(MORSE_L) },
    { 'm', MORSE_M, sizeof(MORSE_M) },
    { 'n', MORSE_N, sizeof(MORSE_N) },
    { 'o', MORSE_O, sizeof(MORSE_O) },
    { 'p', MORSE_P, sizeof(MORSE_P) },
    { 'q', MORSE_Q, sizeof(MORSE_Q) },
    { 'r', MORSE_R, sizeof(MORSE_R) },
    { 's', MORSE_S, sizeof(MORSE_S) },
    { 't', MORSE_T, sizeof(MORSE_T) },
    { 'u', MORSE_U, sizeof(MORSE_U) },
    { 'v', MORSE_V, sizeof(MORSE_V) },
    { 'w', MORSE_W, sizeof(MORSE_W) },
    { 'x', MORSE_X, sizeof(MORSE_X) },
    { 'y', MORSE_Y, sizeof(MORSE_Y) },
    { 'z', MORSE_Z, sizeof(MORSE_Z) }
};

const char UNKNOWN_CHAR = '?';

/**
 * Morse buttons.
 */

const int BTN_DOT_PIN = 2;
const int BTN_DASH_PIN = 3;

const int BUZZ_PIN = 9;
const unsigned int BUZZ_FREQ = 2000;
const unsigned long BUZZ_MS_DOT = 100;
const unsigned long BUZZ_MS_DASH = 300;

Atm_button btnDot;
Atm_button btnDash;

/**
 * Audio FX functions.
 */

bool isTrackPlaying()
{
    return digitalRead(PIN_AUDIO_ACT) == LOW;
}

void playTrack(byte trackPin)
{
    if (isTrackPlaying()) {
        Serial.println(F("Skipping: Audio playing"));
        return;
    }

    Serial.print(F("Playing track on pin: "));
    Serial.println(trackPin);

    digitalWrite(trackPin, LOW);
    pinMode(trackPin, OUTPUT);
    delay(300);
    pinMode(trackPin, INPUT);
}

void initAudioPins()
{
    pinMode(PIN_TRACK_SUCCESS_ONE, INPUT);
    pinMode(PIN_TRACK_SUCCESS_TWO, INPUT);
    pinMode(PIN_AUDIO_ACT, INPUT);
    pinMode(PIN_AUDIO_RST, INPUT);
}

void resetAudio()
{
    Serial.println(F("Audio FX reset"));

    digitalWrite(PIN_AUDIO_RST, LOW);
    pinMode(PIN_AUDIO_RST, OUTPUT);
    delay(10);
    pinMode(PIN_AUDIO_RST, INPUT);

    Serial.println(F("Waiting for Audio FX startup"));

    delay(2000);
}

/**
 * LCD state loop function.
 */

void updateLcd()
{
    const int lcdCols = 16;

    String theStr;

    if (strMorseDecoded.length() == 0 && !isTouched) {
        theStr = STR_DEFAULT;
    } else if (isComplete) {
        theStr = STR_SUCCESS;
    } else {
        theStr = strMorseDecoded;
    }

    int from = theStr.length() - lcdCols;
    from = from < 0 ? 0 : from;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(theStr.substring(from));
}

/**
 * Morse logic functions.
 */

/**
 * Arg and return value are inclusive.
 */
int findLetterEnd(int idxStart)
{
    if (idxStart > (morseBuf.size() - 1)) {
        return -1;
    }

    int idxPivot = idxStart;
    unsigned long diff;

    for (int i = (idxStart + 1); i < morseBuf.size(); i++) {
        diff = morseBuf[i].time - morseBuf[i - 1].time;

        if (diff >= MORSE_LETTER_TIMEOUT_MS) {
            break;
        } else {
            idxPivot = i;
        }
    }

    return idxPivot;
}

int findMorseEntryIndex(int idxStart, int idxEnd)
{
    if (idxStart < 0 || idxEnd >= morseBuf.size() || idxStart > idxEnd) {
        return -1;
    }

    unsigned int idxDelta = idxEnd - idxStart + 1;
    bool isValid;

    for (int i = 0; i < MORSE_DICT_NUM; i++) {
        isValid = true;

        if (morseDict[i].size != idxDelta) {
            continue;
        }

        for (unsigned int j = 0; j < idxDelta; j++) {
            if (morseDict[i].def[j] != morseBuf[idxStart + j].val) {
                isValid = false;
                break;
            }
        }

        if (isValid) {
            return i;
        }
    }

    return -1;
}

void decodeMorseString()
{
    strMorseDecoded = String("");

    if (morseBuf.isEmpty()) {
        return;
    }

    int idxStart = 0;
    int idxEnd;
    int entryIdx;
    char letter;

    do {
        idxEnd = findLetterEnd(idxStart);
        entryIdx = findMorseEntryIndex(idxStart, idxEnd);
        letter = entryIdx >= 0 ? morseDict[entryIdx].val : UNKNOWN_CHAR;
        strMorseDecoded.concat(letter);
        idxStart = idxEnd + 1;
    } while (idxStart < morseBuf.size());
}

bool isDecodedStringValid()
{
    return strMorseDecoded.indexOf(STR_KEY) > -1;
}

void onMorseCompleted()
{
    const unsigned long msShort = 500;
    const unsigned long msLong = 5000;
    const unsigned long msTimeout = 10000;

    Serial.println(F("Completed"));

    isComplete = true;
    updateLcd();
    delay(msShort);
    playTrack(PIN_TRACK_SUCCESS_ONE);

    Serial.println(F("Wait for track #1"));

    unsigned long ini = millis();

    while (isTrackPlaying() && (millis() - ini) < msTimeout) {
        continue;
    }

    Serial.print(F("Waiting "));
    Serial.print(msLong);
    Serial.println(F(" secs"));

    delay(msLong);

    Serial.println(F("Playing track #2"));

    playTrack(PIN_TRACK_SUCCESS_TWO);
}

/**
 * LCD state loop timer.
 */

void onLcdTimer(int idx, int v, int up)
{
    decodeMorseString();

    Serial.print(millis());
    Serial.print(F(":'"));
    Serial.print(strMorseDecoded);
    Serial.println(F("'"));

    updateLcd();

    if (isComplete == false && isDecodedStringValid()) {
        onMorseCompleted();
    }
}

void initLcd()
{
    Serial.println(F("Init LCD"));

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.setBacklight(HIGH);

    Serial.println(F("LCD initialized"));

    lcdTimer
        .begin(LCD_TIMER_MS)
        .repeat(-1)
        .onTimer(onLcdTimer)
        .start();
}

/**
 * Morse buttons functions.
 */

void onPressMorseButton(int idx, int v, int up)
{
    if (v < 1) {
        return;
    }

    isTouched = true;

    if (v > 1) {
        Serial.println(F("Clearing morse buffer"));
        morseBuf.clear();
        return;
    }

    unsigned long now = millis();
    morseBuf.push(MorseItem { now, static_cast<byte>(idx) });

    tone(BUZZ_PIN, BUZZ_FREQ);

    if (idx == MORSE_DOT) {
        Serial.print(now);
        Serial.println(F(":Dot"));
        delay(BUZZ_MS_DOT);
    } else if (idx == MORSE_DASH) {
        Serial.print(now);
        Serial.println(F(":Dash"));
        delay(BUZZ_MS_DASH);
    }

    noTone(BUZZ_PIN);
}

void initMorseButtons()
{
    const int longPressMax = 2;
    const int longPressDelay = 3000;

    btnDot
        .begin(BTN_DOT_PIN)
        .longPress(longPressMax, longPressDelay)
        .onPress(onPressMorseButton, MORSE_DOT);

    btnDash
        .begin(BTN_DASH_PIN)
        .longPress(longPressMax, longPressDelay)
        .onPress(onPressMorseButton, MORSE_DASH);

    pinMode(BUZZ_PIN, OUTPUT);
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initMorseButtons();
    initAudioPins();
    resetAudio();
    initLcd();

    Serial.println(F(">> Starting Morse program"));
}

void loop()
{
    automaton.run();
}