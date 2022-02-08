#include <Arduino.h>
#include <CD74HC4067.h>

// s0 s1 s2 s3
CD74HC4067 muxOne(5, 4, 3, 2);
CD74HC4067 muxTwo(10, 9, 8, 7);

// Select a pin to share with the 16 channels of the CD74HC4067
const int MUX_ONE_SIG = A0;
const int MUX_TWO_SIG = A1;

const int NUM_MUX_INPUTS = 16;

const uint8_t CODE_RETURN = 0xB0;
const uint8_t CODE_DELETE = 0xD4;

const uint8_t MUX_ONE_MAP[NUM_MUX_INPUTS] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'};

const uint8_t MUX_TWO_MAP[NUM_MUX_INPUTS] = {
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', CODE_RETURN, CODE_DELETE, CODE_RETURN, CODE_RETURN, CODE_RETURN, CODE_RETURN};

void setup()
{
  Serial.begin(9600);
  pinMode(MUX_ONE_SIG, INPUT_PULLUP);
  pinMode(MUX_TWO_SIG, INPUT_PULLUP);
}

void loop()
{
  for (int i = 0; i < 16; i++)
  {
    muxOne.channel(i);

    if (digitalRead(MUX_ONE_SIG) == LOW)
    {
      Serial.print(millis());
      Serial.print(F(" :: Channel 0 :: "));
      Serial.println(i);
    }
  }

  for (int i = 0; i < 16; i++)
  {
    muxTwo.channel(i);

    if (digitalRead(MUX_TWO_SIG) == LOW)
    {
      Serial.print(millis());
      Serial.print(F(" :: Channel 1 :: "));
      Serial.println(i);
    }
  }
}
