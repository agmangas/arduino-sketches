#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_VS1053.h>
#include <CircularBuffer.h>
#include <Keypad.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

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

const uint16_t NUM_CODES = 3;
const uint16_t CODE_SIZE = 9;

char codesArr[NUM_CODES][CODE_SIZE] = {
    { '6', '0', '0', '9', '0', '0', '6', '0', '0' },
    { '6', '0', '0', '9', '0', '0', '6', '0', '1' },
    { '6', '0', '0', '9', '0', '0', '6', '0', '2' }
};

const String tracksArr[NUM_CODES] = {
    String("/track01.mp3"),
    String("/track02.mp3"),
    String("/track03.mp3")
};

/**
 * Keypad.
 */

const uint16_t KEY_ROWS = 4;
const uint16_t KEY_COLS = 3;

char keys[KEY_ROWS][KEY_COLS] = {
    { '1', '2', '3' },
    { '4', '5', '6' },
    { '7', '8', '9' },
    { '*', '0', '#' }
};

uint8_t keyRowPins[KEY_ROWS] = { 11, 12, 13, A2 };
uint8_t keyColPins[KEY_COLS] = { A3, A4, A5 };

Keypad kpd = Keypad(
    makeKeymap(keys),
    keyRowPins,
    keyColPins,
    KEY_ROWS,
    KEY_COLS);

CircularBuffer<char, CODE_SIZE> keyBuffer;

/**
 * OLED display.
 */

Adafruit_SH110X display = Adafruit_SH110X(64, 128, &Wire);

typedef struct programState {
} ProgramState;

ProgramState progState;

void initState()
{
}

void updateKeyBuffer()
{
    char key = kpd.getKey();

    if (key != NO_KEY) {
        Serial.print("Key: ");
        Serial.println(key);
        keyBuffer.push(key);
    }
}

bool inKeyBuffer(char* val, size_t valSize)
{
    if (keyBuffer.size() < valSize) {
        return false;
    }

    for (int i = 0; i < keyBuffer.size(); i++) {
        if (keyBuffer[i] != val[i]) {
            return false;
        }
    }

    return true;
}

int findInBuffer()
{
    for (int i = 0; i < NUM_CODES; i++) {
        if (inKeyBuffer(codesArr[i], CODE_SIZE)) {
            return i;
        }
    }

    return -1;
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
        Serial.println("VS1053 not found");
    }

    Serial.println("VS1053 OK");

    if (!SD.begin(CARDCS)) {
        Serial.println("SD failed or not found");
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

void initDisplay()
{
    Serial.println("Initializing OLED display");

    // Address 0x3C by default
    display.begin(0x3C, true);

    Serial.println("OLED display init OK");

    display.setTextColor(SH110X_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Initialized");
    display.display();
}

void findAndPlay()
{
    if (!keyBuffer.isFull()) {
        return;
    }

    int codeIdx = findInBuffer();

    keyBuffer.clear();

    if (codeIdx < 0 || !musicPlayer.stopped()) {
        return;
    }

    playTrack(tracksArr[codeIdx]);
}

void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);

    initState();
    initAudio();
    initDisplay();

    Serial.println(">> Maletin-fono");
}

void loop()
{
    updateKeyBuffer();
    findAndPlay();
}
