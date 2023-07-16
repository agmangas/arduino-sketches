#include <Arduino.h>
#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

/**
 * Controller buttons.
 */

const uint8_t BUTTONS_NUM = 5;
const uint8_t BUTTONS_PINS[BUTTONS_NUM] = {3, 4, 5, 6, 7};
Atm_button buttons[BUTTONS_NUM];

const uint8_t BUTTONS_BUF_SIZE = 10;
CircularBuffer<uint8_t, BUTTONS_BUF_SIZE> buttonBuf;

const int BUTTONS_DEBOUNCE_MS = 100;

/**
 * LEDs for the controller buttons.
 */

const uint8_t LEDS_PER_BUTTON = 1;
const uint16_t LED_BUTTONS_NUM = BUTTONS_NUM * LEDS_PER_BUTTON;
const int16_t LED_BUTTONS_PIN = 8;

Adafruit_NeoPixel ledButtons = Adafruit_NeoPixel(
    LED_BUTTONS_NUM,
    LED_BUTTONS_PIN,
    NEO_RGB + NEO_KHZ800);

/**
 * LEDs for the space invaders matrix and signal.
 */

const uint8_t INVADERS_WIDTH = 5;
const uint8_t INVADERS_HEIGHT = 3;
const uint8_t INVADERS_TOTAL = INVADERS_WIDTH * INVADERS_HEIGHT;
const uint8_t LEDS_PER_INVADER = 1;
const uint16_t NUM_LEDS_INVADERS = INVADERS_TOTAL * LEDS_PER_INVADER;

const uint8_t NUM_LEDS_SIGNAL = 1;

const uint16_t LED_MATRIX_TOTAL_NUM = NUM_LEDS_INVADERS + NUM_LEDS_SIGNAL;
const int16_t LED_MATRIX_PIN = 9;

Adafruit_NeoPixel ledInvaders = Adafruit_NeoPixel(
    LED_MATRIX_TOTAL_NUM,
    LED_MATRIX_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

const uint32_t TIMER_STATE_MS = 200;

Atm_timer timerState;

typedef struct programState
{
} ProgramState;

ProgramState progState = {};

void cleanState()
{
}

void showLedStartEffect()
{
  const uint32_t color = Adafruit_NeoPixel::Color(200, 200, 200);
  const uint8_t numIters = 3;
  const unsigned long delayMs = 500;

  for (uint8_t i = 0; i < numIters; i++)
  {
    ledInvaders.fill(color);
    ledInvaders.show();
    ledButtons.fill(color);
    ledButtons.show();

    delay(delayMs);

    ledInvaders.clear();
    ledInvaders.show();
    ledButtons.clear();
    ledButtons.show();

    delay(delayMs);
  }

  ledInvaders.clear();
  ledInvaders.show();
  ledButtons.clear();
  ledButtons.show();
}

void initLeds()
{
  const uint8_t ledBrightness = 230;

  ledButtons.begin();
  ledButtons.setBrightness(ledBrightness);
  ledButtons.clear();
  ledButtons.show();

  ledInvaders.begin();
  ledInvaders.setBrightness(ledBrightness);
  ledInvaders.clear();
  ledInvaders.show();
}

void onPress(int idx, int v, int up)
{
  Serial.print(F("Press:"));
  Serial.println(idx);
  buttonBuf.push(idx);
}

void initButtons()
{
  for (int i = 0; i < BUTTONS_NUM; i++)
  {
    buttons[i]
        .begin(BUTTONS_PINS[i])
        .debounce(BUTTONS_DEBOUNCE_MS)
        .onPress(onPress, i);
  }
}

void onTimerState(int idx, int v, int up)
{
}

void initTimerState()
{
  timerState
      .begin(TIMER_STATE_MS)
      .repeat(-1)
      .onTimer(onTimerState)
      .start();
}

void setup()
{
  cleanState();
  initButtons();
  initTimerState();
  initLeds();
  showLedStartEffect();
}

void loop()
{
  automaton.run();
}