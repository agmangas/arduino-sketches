#include <Automaton.h>
#include <CircularBuffer.h>

/**
 * Morse buffer.
 */

typedef struct morseItem
{
    unsigned long time;
    byte val;
} MorseItem;

const byte MORSE_DOT = 0;
const byte MORSE_DASH = 1;

const int MORSE_LETTER_TIMEOUT_MS = 1500;
const int MORSE_BUF_SIZE = 140;

CircularBuffer<MorseItem, MORSE_BUF_SIZE> morseBuf;

/**
 * Program state.
 */

String strMorseDecoded = String("");
String strMorseKey = String("mono");
String strVictory = String("OK");
bool isComplete = false;
unsigned long printCounter = 0;
const unsigned long PRINT_MOD = 2000;

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

byte MORSE_A[] = {MORSE_DOT, MORSE_DASH};
byte MORSE_B[] = {MORSE_DASH, MORSE_DOT, MORSE_DOT, MORSE_DOT};
byte MORSE_C[] = {MORSE_DASH, MORSE_DOT, MORSE_DASH, MORSE_DOT};
byte MORSE_D[] = {MORSE_DASH, MORSE_DOT, MORSE_DOT};
byte MORSE_E[] = {MORSE_DOT};
byte MORSE_F[] = {MORSE_DOT, MORSE_DOT, MORSE_DASH, MORSE_DOT};
byte MORSE_G[] = {MORSE_DASH, MORSE_DASH, MORSE_DOT};
byte MORSE_H[] = {MORSE_DOT, MORSE_DOT, MORSE_DOT, MORSE_DOT};
byte MORSE_I[] = {MORSE_DOT, MORSE_DOT};
byte MORSE_J[] = {MORSE_DOT, MORSE_DASH, MORSE_DASH, MORSE_DASH};
byte MORSE_K[] = {MORSE_DASH, MORSE_DOT, MORSE_DASH};
byte MORSE_L[] = {MORSE_DOT, MORSE_DASH, MORSE_DOT, MORSE_DOT};
byte MORSE_M[] = {MORSE_DASH, MORSE_DASH};
byte MORSE_N[] = {MORSE_DASH, MORSE_DOT};
byte MORSE_O[] = {MORSE_DASH, MORSE_DASH, MORSE_DASH};
byte MORSE_P[] = {MORSE_DOT, MORSE_DASH, MORSE_DASH, MORSE_DOT};
byte MORSE_Q[] = {MORSE_DASH, MORSE_DASH, MORSE_DOT, MORSE_DASH};
byte MORSE_R[] = {MORSE_DOT, MORSE_DASH, MORSE_DOT};
byte MORSE_S[] = {MORSE_DOT, MORSE_DOT, MORSE_DOT};
byte MORSE_T[] = {MORSE_DASH};
byte MORSE_U[] = {MORSE_DOT, MORSE_DOT, MORSE_DASH};
byte MORSE_V[] = {MORSE_DOT, MORSE_DOT, MORSE_DOT, MORSE_DASH};
byte MORSE_W[] = {MORSE_DOT, MORSE_DASH, MORSE_DASH};
byte MORSE_X[] = {MORSE_DASH, MORSE_DOT, MORSE_DOT, MORSE_DASH};
byte MORSE_Y[] = {MORSE_DASH, MORSE_DOT, MORSE_DASH, MORSE_DASH};
byte MORSE_Z[] = {MORSE_DASH, MORSE_DASH, MORSE_DOT, MORSE_DOT};

typedef struct morseDictEntry
{
    char val;
    byte *def;
    size_t size;
} MorseDictEntry;

const int MORSE_DICT_NUM = 26;

MorseDictEntry morseDict[MORSE_DICT_NUM] = {
    {'a', MORSE_A, sizeof(MORSE_A)},
    {'b', MORSE_B, sizeof(MORSE_B)},
    {'c', MORSE_C, sizeof(MORSE_C)},
    {'d', MORSE_D, sizeof(MORSE_D)},
    {'e', MORSE_E, sizeof(MORSE_E)},
    {'f', MORSE_F, sizeof(MORSE_F)},
    {'g', MORSE_G, sizeof(MORSE_G)},
    {'h', MORSE_H, sizeof(MORSE_H)},
    {'i', MORSE_I, sizeof(MORSE_I)},
    {'j', MORSE_J, sizeof(MORSE_J)},
    {'k', MORSE_K, sizeof(MORSE_K)},
    {'l', MORSE_L, sizeof(MORSE_L)},
    {'m', MORSE_M, sizeof(MORSE_M)},
    {'n', MORSE_N, sizeof(MORSE_N)},
    {'o', MORSE_O, sizeof(MORSE_O)},
    {'p', MORSE_P, sizeof(MORSE_P)},
    {'q', MORSE_Q, sizeof(MORSE_Q)},
    {'r', MORSE_R, sizeof(MORSE_R)},
    {'s', MORSE_S, sizeof(MORSE_S)},
    {'t', MORSE_T, sizeof(MORSE_T)},
    {'u', MORSE_U, sizeof(MORSE_U)},
    {'v', MORSE_V, sizeof(MORSE_V)},
    {'w', MORSE_W, sizeof(MORSE_W)},
    {'x', MORSE_X, sizeof(MORSE_X)},
    {'y', MORSE_Y, sizeof(MORSE_Y)},
    {'z', MORSE_Z, sizeof(MORSE_Z)}};

const char UNKNOWN_CHAR = '?';

/**
 * Morse buttons.
 */

const int BTN_DOT_PIN = 2;
const int BTN_DASH_PIN = 3;

const int BUZZ_PIN = 9;
const unsigned int BUZZ_FREQ = 1000;
const unsigned long BUZZ_MS_DOT = 100;
const unsigned long BUZZ_MS_DASH = 300;

Atm_button btnDot;
Atm_button btnDash;

/**
 * Morse logic functions.
 */

/**
 * Arg and return value are inclusive.
 */
int findLetterEnd(int idxStart)
{
    if (idxStart > (morseBuf.size() - 1))
    {
        return -1;
    }

    int idxPivot = idxStart;
    unsigned long diff;

    for (int i = (idxStart + 1); i < morseBuf.size(); i++)
    {
        diff = morseBuf[i].time - morseBuf[i - 1].time;

        if (diff >= MORSE_LETTER_TIMEOUT_MS)
        {
            break;
        }
        else
        {
            idxPivot = i;
        }
    }

    return idxPivot;
}

int findMorseEntryIndex(int idxStart, int idxEnd)
{
    if (idxStart < 0 ||
        idxEnd >= morseBuf.size() ||
        idxStart > idxEnd)
    {
        return -1;
    }

    int idxDelta = idxEnd - idxStart + 1;
    bool isValid;

    for (int i = 0; i < MORSE_DICT_NUM; i++)
    {
        isValid = true;

        if (morseDict[i].size != idxDelta)
        {
            continue;
        }

        for (int j = 0; j < idxDelta; j++)
        {
            if (morseDict[i].def[j] !=
                morseBuf[idxStart + j].val)
            {
                isValid = false;
                break;
            }
        }

        if (isValid)
        {
            return i;
        }
    }

    return -1;
}

void decodeMorseString()
{
    strMorseDecoded = String("");

    if (morseBuf.isEmpty())
    {
        return;
    }

    int idxStart = 0;
    int idxEnd;
    int entryIdx;
    char letter;

    do
    {
        idxEnd = findLetterEnd(idxStart);
        entryIdx = findMorseEntryIndex(idxStart, idxEnd);
        letter = entryIdx >= 0 ? morseDict[entryIdx].val : UNKNOWN_CHAR;
        strMorseDecoded.concat(letter);
        idxStart = idxEnd + 1;
    } while (idxStart < morseBuf.size());
}

bool isDecodedStringValid()
{
    return strMorseDecoded.indexOf(strMorseKey) > -1;
}

void decodeAndCheck()
{
    decodeMorseString();

    printCounter++;

    if (printCounter % PRINT_MOD == 0)
    {
        Serial.print(millis());
        Serial.print(F(":'"));
        Serial.print(strMorseDecoded);
        Serial.println(F("'"));
    }

    if (isComplete == false && isDecodedStringValid())
    {
        onMorseCompleted();
    }
}

void onMorseCompleted()
{
    const unsigned long msShort = 500;
    const unsigned long msLong = 2000;

    // ToDo: Write victory message

    isComplete = true;
    delay(msShort);
    playTrack(PIN_TRACK_SUCCESS_ONE);

    while (isTrackPlaying())
    {
        continue;
    }

    delay(msLong);
    playTrack(PIN_TRACK_SUCCESS_TWO);
}

/**
 * Morse buttons functions.
 */

void onPressMorseButton(int idx, int v, int up)
{
    unsigned long now = millis();
    morseBuf.push(MorseItem{now, idx});

    tone(BUZZ_PIN, BUZZ_FREQ);

    if (idx == MORSE_DOT)
    {
        Serial.print(now);
        Serial.println(F(":Dot"));
        delay(BUZZ_MS_DOT);
    }
    else if (idx == MORSE_DASH)
    {
        Serial.print(now);
        Serial.println(F(":Dash"));
        delay(BUZZ_MS_DASH);
    }

    noTone(BUZZ_PIN);
}

void initMorseButtons()
{
    btnDot
        .begin(BTN_DOT_PIN)
        .onPress(onPressMorseButton, MORSE_DOT);

    btnDash
        .begin(BTN_DASH_PIN)
        .onPress(onPressMorseButton, MORSE_DASH);

    pinMode(BUZZ_PIN, OUTPUT);
}

/**
 * Audio FX functions.
 */

void playTrack(byte trackPin)
{
    if (isTrackPlaying())
    {
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

bool isTrackPlaying()
{
    return digitalRead(PIN_AUDIO_ACT) == LOW;
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
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initMorseButtons();
    initAudioPins();
    resetAudio();

    Serial.println(F(">> Starting Morse program"));
}

void loop()
{
    automaton.run();
    decodeAndCheck();
}