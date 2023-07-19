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
 * Colors.
 */

const uint32_t RED = Adafruit_NeoPixel::Color(255, 0, 0);
const uint32_t GREEN = Adafruit_NeoPixel::Color(0, 255, 0);
const uint32_t BLUE = Adafruit_NeoPixel::Color(0, 0, 255);
const uint32_t YELLOW = Adafruit_NeoPixel::Color(255, 255, 0);

const uint8_t NUM_COLORS_FIRST_PHASE = 4;
const uint8_t NUM_COLORS_SECOND_PHASE = 3;

const uint32_t COLORS_FIRST_PHASE[NUM_COLORS_FIRST_PHASE] = {
    RED,
    GREEN,
    BLUE,
    YELLOW};

const uint8_t COLORS_FIRST_PHASE_KEY[BUTTONS_NUM] = {
    0, 1, 0, 3, 2};

const uint32_t COLORS_SECOND_PHASE[NUM_COLORS_SECOND_PHASE] = {
    RED,
    GREEN,
    BLUE};

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
 * LEDs for the space invaders matrix.
 */

const uint8_t INVADERS_WIDTH = 5;
const uint8_t INVADERS_HEIGHT = 3;
const uint8_t LEDS_PER_INVADER = 1;
const uint8_t INVADERS_TOTAL = INVADERS_WIDTH * INVADERS_HEIGHT;
const uint16_t NUM_LEDS_INVADERS = INVADERS_TOTAL * LEDS_PER_INVADER;
const int16_t LED_MATRIX_PIN = 9;

Adafruit_NeoPixel ledInvaders = Adafruit_NeoPixel(
    NUM_LEDS_INVADERS,
    LED_MATRIX_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * LEDs for the activation signal.
 */

const uint16_t LED_SIGNAL_NUM = 1;
const int16_t LED_SIGNAL_PIN = 10;

Adafruit_NeoPixel ledSignal = Adafruit_NeoPixel(
    LED_SIGNAL_NUM,
    LED_SIGNAL_PIN,
    NEO_GRB + NEO_KHZ800);

/**
 * Program state.
 */

const uint32_t TIMER_GENERAL_MS = 200;
Atm_timer timerGeneral;

const uint32_t TIMER_SECOND_PHASE_MS = 3000;
Atm_timer timerSecondPhase;

uint8_t buttonColorIdxs[BUTTONS_NUM];
uint8_t invaderColorIdxs[INVADERS_TOTAL];

typedef struct programState
{
  bool isSecondPhase;
  uint8_t *buttonColorIdxs;
  uint8_t *invaderColorIdxs;
  uint8_t signalColorIdx;
} ProgramState;

ProgramState progState = {
    .isSecondPhase = false,
    .buttonColorIdxs = buttonColorIdxs,
    .invaderColorIdxs = invaderColorIdxs,
    .signalColorIdx = 0};

void cleanState()
{
  progState.isSecondPhase = false;
  progState.signalColorIdx = 0;

  for (uint8_t i = 0; i < BUTTONS_NUM; i++)
  {
    progState.buttonColorIdxs[i] = 0;
  }

  for (uint8_t i = 0; i < INVADERS_TOTAL; i++)
  {
    progState.invaderColorIdxs[i] = 0;
  }
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

  ledSignal.begin();
  ledSignal.setBrightness(ledBrightness);
  ledSignal.clear();
  ledSignal.show();
}

void showInvaderLeds()
{
  if (!progState.isSecondPhase)
  {
    return;
  }

  ledInvaders.clear();

  for (uint8_t idxInvader = 0; idxInvader < INVADERS_TOTAL; idxInvader++)
  {
    const uint8_t colorIdx = progState.invaderColorIdxs[idxInvader];

    uint32_t color = Adafruit_NeoPixel::Color(0, 0, 0);

    if (colorIdx < NUM_COLORS_SECOND_PHASE)
    {
      color = COLORS_SECOND_PHASE[colorIdx];
    }

    const uint8_t baseLedIdx = idxInvader * LEDS_PER_INVADER;

    for (uint8_t pivot = 0; pivot < LEDS_PER_INVADER; pivot++)
    {
      ledInvaders.setPixelColor(baseLedIdx + pivot, color);
    }
  }

  ledInvaders.show();
}

void showSignalLeds()
{
  if (!progState.isSecondPhase)
  {
    return;
  }

  const uint8_t colorIdx = progState.signalColorIdx;
  uint32_t color = Adafruit_NeoPixel::Color(0, 0, 0);

  if (colorIdx < NUM_COLORS_SECOND_PHASE)
  {
    color = COLORS_SECOND_PHASE[colorIdx];
  }

  ledSignal.clear();
  ledSignal.fill(color);
  ledSignal.show();
}

void showButtonLeds()
{
  ledButtons.clear();

  for (uint8_t idxButton = 0; idxButton < BUTTONS_NUM; idxButton++)
  {
    const uint8_t colorIdx = progState.buttonColorIdxs[idxButton];

    uint32_t color = Adafruit_NeoPixel::Color(0, 0, 0);

    if (!progState.isSecondPhase && colorIdx < NUM_COLORS_FIRST_PHASE)
    {
      color = COLORS_FIRST_PHASE[colorIdx];
    }
    else if (progState.isSecondPhase && colorIdx < NUM_COLORS_SECOND_PHASE)
    {
      color = COLORS_SECOND_PHASE[colorIdx];
    }

    const uint8_t baseLedIdx = idxButton * LEDS_PER_BUTTON;

    for (uint8_t pivot = 0; pivot < LEDS_PER_BUTTON; pivot++)
    {
      ledButtons.setPixelColor(baseLedIdx + pivot, color);
    }
  }

  ledButtons.show();
}

uint8_t randomSecondPhaseColorIdx()
{
  return random(0, NUM_COLORS_SECOND_PHASE);
}

void secondPhaseRandomize()
{
  progState.signalColorIdx = randomSecondPhaseColorIdx();

  for (uint8_t idxButton = 0; idxButton < BUTTONS_NUM; idxButton++)
  {
    progState.buttonColorIdxs[idxButton] = randomSecondPhaseColorIdx();
  }
}

bool isFirstPhaseConfigurationCorrect()
{
  if (progState.isSecondPhase)
  {
    return false;
  }

  for (uint8_t idxButton = 0; idxButton < BUTTONS_NUM; idxButton++)
  {
    if (progState.buttonColorIdxs[idxButton] != COLORS_FIRST_PHASE_KEY[idxButton])
    {
      return false;
    }
  }

  return true;
}

void checkTransitionToSecondPhase()
{
  if (progState.isSecondPhase || !isFirstPhaseConfigurationCorrect())
  {
    return;
  }

  Serial.println(F("Transitioning to second phase"));

  for (uint8_t idxButton = 0; idxButton < BUTTONS_NUM; idxButton++)
  {
    progState.buttonColorIdxs[idxButton] = randomSecondPhaseColorIdx();
  }

  progState.signalColorIdx = randomSecondPhaseColorIdx();

  for (uint8_t idxInvader = 0; idxInvader < INVADERS_TOTAL; idxInvader++)
  {
    progState.invaderColorIdxs[idxInvader] = randomSecondPhaseColorIdx();
  }

  progState.isSecondPhase = true;
}

void onPress(int idx, int v, int up)
{
  Serial.print(F("Press:"));
  Serial.println(idx);

  buttonBuf.push(idx);

  if (!progState.isSecondPhase)
  {
    progState.buttonColorIdxs[idx] =
        (progState.buttonColorIdxs[idx] + 1) % NUM_COLORS_FIRST_PHASE;
  }
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

void onTimerGeneral(int idx, int v, int up)
{
  showButtonLeds();
  checkTransitionToSecondPhase();
  showInvaderLeds();
  showSignalLeds();
}

void onTimerSecondPhase(int idx, int v, int up)
{
  if (!progState.isSecondPhase)
  {
    return;
  }

  secondPhaseRandomize();
}

void initTimers()
{
  timerGeneral
      .begin(TIMER_GENERAL_MS)
      .repeat(-1)
      .onTimer(onTimerGeneral)
      .start();

  timerSecondPhase
      .begin(TIMER_SECOND_PHASE_MS)
      .repeat(-1)
      .onTimer(onTimerSecondPhase)
      .start();
}

void setup()
{
  cleanState();
  initButtons();
  initTimers();
  initLeds();
  showLedStartEffect();
}

void loop()
{
  automaton.run();
}