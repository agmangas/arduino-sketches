#include <Adafruit_VS1053.h>
#include <CircularBuffer.h>
#include <Keypad.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
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

const String descriptionsArr[NUM_CODES] = {
    String("Descripcion 01"),
    String("Descripcion 02"),
    String("Descripcion 03")
};

/**
 * Keypad.
 */

const uint16_t KEY_ROWS = 4;
const uint16_t KEY_COLS = 4;

char keys[KEY_ROWS][KEY_COLS] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

uint8_t keyRowPins[KEY_ROWS] = { 1, 12, 11, A3 };
uint8_t keyColPins[KEY_COLS] = { 4, A5, 0, A4 };

Keypad kpd = Keypad(
    makeKeymap(keys),
    keyRowPins,
    keyColPins,
    KEY_ROWS,
    KEY_COLS);

CircularBuffer<char, CODE_SIZE> keyBuffer;

const uint8_t PIN_HANGUP = A2;
const String MSG_DEFAULT = String("Telefono viejuno");

/**
 * OLED display.
 */

const uint8_t LCD_COLS = 16;
const uint8_t LCD_LINES = 2;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void printDisplay(String content)
{
    int from = content.length() - LCD_COLS;
    from = from < 0 ? 0 : from;
    String contentPrint = content.substring(from);

    Serial.print("Print: ");
    Serial.println(contentPrint);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(contentPrint);
}

/**
 * Program state.
 */

bool hangState;
unsigned long hangEdgeMillis;

void initState()
{
    hangState = true;
    hangEdgeMillis = 0;
}

void initDisplay()
{
    Serial.println("Initializing LCD");

    lcd.begin(LCD_COLS, LCD_LINES);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.setBacklight(HIGH);

    Serial.println("LCD initialized OK");

    printDisplay(MSG_DEFAULT);
}

void displayKeyBuffer()
{
    String val = String("");

    for (int i = 0; i < keyBuffer.size(); i++) {
        val.concat(keyBuffer[i]);
    }

    printDisplay(val);
}

void updateKeyBuffer()
{
    char key = kpd.getKey();

    if (key != NO_KEY) {
        Serial.print("Key: ");
        Serial.println(key);
        keyBuffer.push(key);
        displayKeyBuffer();
    }
}

bool inKeyBuffer(char* val, size_t valSize)
{
    if (keyBuffer.size() < valSize) {
        return false;
    }

    for (uint16_t i = 0; i < valSize; i++) {
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

void findAndPlay()
{
    if (!keyBuffer.isFull()) {
        return;
    }

    int codeIdx = findInBuffer();

    keyBuffer.clear();

    if (codeIdx < 0) {
        Serial.println("Unknown code");
        return;
    }

    Serial.print("Found code: ");
    Serial.println(codeIdx);

    if (!musicPlayer.stopped()) {
        musicPlayer.stopPlaying();
    }

    playTrack(tracksArr[codeIdx]);
    printDisplay(descriptionsArr[codeIdx]);
}

bool isPhoneHungUp()
{
    return digitalRead(PIN_HANGUP) == LOW;
}

void onHangUp()
{
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Hang up");

    if (!musicPlayer.stopped()) {
        Serial.println("Stopping music");
        musicPlayer.stopPlaying();
    }

    Serial.println("Clearing buffer");
    keyBuffer.clear();

    printDisplay(MSG_DEFAULT);
}

void updateHangState()
{
    const unsigned long debounceMs = 300;

    bool currHangState = isPhoneHungUp();

    if (hangState == currHangState) {
        return;
    }

    unsigned long thresholdMillis = hangEdgeMillis + debounceMs;

    if (millis() < thresholdMillis) {
        return;
    }

    Serial.print(hangState ? "HUNG" : "ON");
    Serial.print(" -> ");
    Serial.println(currHangState ? "HUNG" : "ON");

    if (currHangState == true) {
        onHangUp();
    }

    hangState = currHangState;
    hangEdgeMillis = millis();
}

void setup()
{
    Serial.begin(9600);

    initState();
    initAudio();
    initDisplay();
    pinMode(PIN_HANGUP, INPUT_PULLUP);

    Serial.println(">> Maletin-fono");
}

void loop()
{
    updateKeyBuffer();
    findAndPlay();
    updateHangState();
}
