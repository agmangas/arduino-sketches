#include <SerialRFID.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

/**
  Structs.
*/

typedef struct programState {
  uint8_t stage;
} ProgramState;

ProgramState progState = {
  .stage = 0
};

/**
   Pins.
*/

const byte SECONDARY_RFID_01_RX = 9;
const byte SECONDARY_RFID_01_TX = 8;
const byte SECONDARY_RFID_02_RX = 7;
const byte SECONDARY_RFID_02_TX = 6;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
