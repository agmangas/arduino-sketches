#include <Arduino.h>
#include <Automaton.h>
#include <Adafruit_NeoPixel.h>
#include <CircularBuffer.h>

/**
 * Controller buttons.
 */

const uint8_t BUTTONS_NUM = 4;
const uint8_t BUTTONS_PINS[BUTTONS_NUM] = {A0, A1, A2, A3};
Atm_button buttons[BUTTONS_NUM];

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
    0, 1, 3, 2};

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
const uint8_t INVADERS_HEIGHT = 4;
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
    NEO_RGB + NEO_KHZ800);

/**
 * Audio FX.
 */

const uint8_t PIN_AUDIO_ACT = 11;
const uint8_t PIN_AUDIO_RST = 12;
const uint8_t PIN_AUDIO_TRACK_ERROR = 3;
const uint8_t PIN_AUDIO_TRACK_VICTORY = 4;
const uint8_t PIN_AUDIO_SECOND_PHASE = 5;
const unsigned long AUDIO_PLAY_DELAY_MS = 200;

/**
 * Relays.
 */

const int PIN_RELAY = 7;

/**
 * Program state.
 */

const uint8_t BUTTONS_BUF_SIZE = 5;
CircularBuffer<uint8_t, BUTTONS_BUF_SIZE> buttonBuf;

const uint8_t AUDIO_BUF_SIZE = 2;
CircularBuffer<uint8_t, AUDIO_BUF_SIZE> audioPinsQueue;

const uint32_t TIMER_GENERAL_MS = 200;
Atm_timer timerGeneral;

const uint32_t TIMER_SECOND_PHASE_MS = 2500;
Atm_timer timerSecondPhase;

uint8_t buttonColorIdxs[BUTTONS_NUM];
uint8_t invaderColorIdxs[INVADERS_TOTAL];
bool invaderFlags[INVADERS_TOTAL];

typedef struct programState
{
  bool isSecondPhase;
  uint8_t *buttonColorIdxs;
  uint8_t *invaderColorIdxs;
  uint8_t signalColorIdx;
  bool *invaderFlags;
  unsigned long audioPlayMillis;
} ProgramState;

ProgramState progState = {
    .isSecondPhase = false,
    .buttonColorIdxs = buttonColorIdxs,
    .invaderColorIdxs = invaderColorIdxs,
    .signalColorIdx = 0,
    .invaderFlags = invaderFlags,
    .audioPlayMillis = 0};

void cleanState()
{
  progState.isSecondPhase = false;
  progState.signalColorIdx = 0;
  progState.audioPlayMillis = 0;

  for (uint8_t i = 0; i < BUTTONS_NUM; i++)
  {
    progState.buttonColorIdxs[i] = 0;
  }

  for (uint8_t i = 0; i < INVADERS_TOTAL; i++)
  {
    progState.invaderColorIdxs[i] = 0;
    progState.invaderFlags[i] = false;
  }

  buttonBuf.clear();
  audioPinsQueue.clear();
}

/**
 * Functions to manage the relay.
 */

void lockRelay()
{
  digitalWrite(PIN_RELAY, LOW);
}

void openRelay()
{
  digitalWrite(PIN_RELAY, HIGH);
}

void initRelays()
{
  pinMode(PIN_RELAY, OUTPUT);
  lockRelay();
}

/**
 * Functions to configure and control the audio FX.
 */

void initAudioPins()
{
  pinMode(PIN_AUDIO_TRACK_ERROR, INPUT);
  pinMode(PIN_AUDIO_TRACK_VICTORY, INPUT);
  pinMode(PIN_AUDIO_SECOND_PHASE, INPUT);
  pinMode(PIN_AUDIO_ACT, INPUT);
  pinMode(PIN_AUDIO_RST, INPUT);
}

void resetAudio()
{
  const unsigned long resetDelayMs = 100;
  const unsigned long waitDelayMs = 2000;

  Serial.println(F("Audio FX reset"));

  digitalWrite(PIN_AUDIO_RST, LOW);
  pinMode(PIN_AUDIO_RST, OUTPUT);
  delay(resetDelayMs);
  pinMode(PIN_AUDIO_RST, INPUT);

  Serial.println(F("Waiting for Audio FX startup"));

  delay(waitDelayMs);
}

void initAudio()
{
  initAudioPins();
  resetAudio();
}

bool isTrackPlaying()
{
  return progState.audioPlayMillis > 0 || digitalRead(PIN_AUDIO_ACT) == LOW;
}

void playTrack(uint8_t trackPin, bool async = true)
{
  if (isTrackPlaying())
  {
    Serial.println(F("Skipping: Audio still playing"));
    return;
  }

  digitalWrite(trackPin, LOW);
  pinMode(trackPin, OUTPUT);

  if (async)
  {
    progState.audioPlayMillis = millis();
  }
  else
  {
    progState.audioPlayMillis = 0;
    delay(AUDIO_PLAY_DELAY_MS);
    pinMode(trackPin, INPUT);
  }
}

void clearAudioPins()
{
  progState.audioPlayMillis = 0;
  pinMode(PIN_AUDIO_TRACK_ERROR, INPUT);
  pinMode(PIN_AUDIO_TRACK_VICTORY, INPUT);
  pinMode(PIN_AUDIO_SECOND_PHASE, INPUT);
}

void checkClearAudioPins()
{
  if (progState.audioPlayMillis == 0)
  {
    return;
  }

  unsigned long now = millis();
  unsigned long diffMs = now - progState.audioPlayMillis;

  if (diffMs >= AUDIO_PLAY_DELAY_MS)
  {
    Serial.println(F("Clearing audio pins"));
    clearAudioPins();
  }
}

void processAudioQueue()
{
  if (audioPinsQueue.isEmpty() || isTrackPlaying())
  {
    return;
  }

  Serial.print(F("Audio queue size: "));
  Serial.println(audioPinsQueue.size());

  int trackPin = audioPinsQueue.shift();
  playTrack(trackPin);
}

void enqueueTrack(uint8_t trackPin)
{
  audioPinsQueue.push(trackPin);
}

/**
 * Functions to update the state of the LEDs.
 */

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
    ledSignal.fill(color);
    ledSignal.show();

    delay(delayMs);

    ledInvaders.clear();
    ledInvaders.show();
    ledButtons.clear();
    ledButtons.show();
    ledSignal.clear();
    ledSignal.show();

    delay(delayMs);
  }

  ledInvaders.clear();
  ledInvaders.show();
  ledButtons.clear();
  ledButtons.show();
  ledSignal.clear();
  ledSignal.show();
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

    if (colorIdx < NUM_COLORS_SECOND_PHASE && !progState.invaderFlags[idxInvader])
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

/**
 * Functions to calculate and update the internal state of the game.
 */

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

  uint8_t randomButtonIdx = random(0, BUTTONS_NUM);
  progState.buttonColorIdxs[randomButtonIdx] = progState.signalColorIdx;
}

bool isInvadersMatrixComplete()
{
  for (uint8_t idxInvader = 0; idxInvader < INVADERS_TOTAL; idxInvader++)
  {
    if (!progState.invaderFlags[idxInvader])
    {
      return false;
    }
  }

  return true;
}

void checkVictory()
{
  if (!progState.isSecondPhase || !isInvadersMatrixComplete())
  {
    return;
  }

  const unsigned long delayMs = 200;
  const unsigned long endMillis = millis() + 12000;

  clearAudioPins();
  playTrack(PIN_AUDIO_TRACK_VICTORY, false);

  openRelay();

  uint32_t color;
  uint8_t colorIdx;

  while (isTrackPlaying() || (millis() < endMillis))
  {
    colorIdx = random(0, NUM_COLORS_SECOND_PHASE);
    color = COLORS_SECOND_PHASE[colorIdx];

    ledButtons.fill(color);
    ledInvaders.fill(color);
    ledButtons.show();
    ledInvaders.show();

    delay(delayMs);

    ledButtons.clear();
    ledInvaders.clear();
    ledButtons.show();
    ledInvaders.show();

    delay(delayMs);
  }

  cleanState();
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

  enqueueTrack(PIN_AUDIO_SECOND_PHASE);
  progState.isSecondPhase = true;
}

bool isButtonSignalMatch(uint8_t idxButton)
{
  return progState.buttonColorIdxs[idxButton] == progState.signalColorIdx;
}

void setFlagForInvaderByColorIdx(uint8_t colorIdx)
{
  Serial.print(F("Setting invader flag with color: "));
  Serial.println(colorIdx);

  for (uint8_t idxInvader = 0; idxInvader < INVADERS_TOTAL; idxInvader++)
  {
    if (progState.invaderColorIdxs[idxInvader] == colorIdx && !progState.invaderFlags[idxInvader])
    {
      progState.invaderFlags[idxInvader] = true;
      return;
    }
  }
}

void runSecondPhaseErrorEffect()
{
  Serial.println(F("Running second phase error"));

  const unsigned long delayMs = 200;
  const uint32_t white = Adafruit_NeoPixel::Color(255, 255, 255);
  const unsigned long endMillis = millis() + 1500;

  clearAudioPins();
  playTrack(PIN_AUDIO_TRACK_ERROR, false);

  while (isTrackPlaying() || (millis() < endMillis))
  {
    ledButtons.fill(white);
    ledButtons.show();

    delay(delayMs);

    ledButtons.clear();
    ledButtons.show();

    delay(delayMs);
  }
}

void rotateFirstPhaseButton(uint8_t idxButton)
{
  progState.buttonColorIdxs[idxButton] =
      (progState.buttonColorIdxs[idxButton] + 1) % NUM_COLORS_FIRST_PHASE;
}

/**
 * Functions to handle button presses.
 */

void onPress(int idxButton, int v, int up)
{
  Serial.print(F("Press: "));
  Serial.println(idxButton);

  buttonBuf.push(idxButton);

  if (!progState.isSecondPhase)
  {
    rotateFirstPhaseButton(idxButton);
  }
  else if (isButtonSignalMatch(idxButton))
  {
    setFlagForInvaderByColorIdx(progState.signalColorIdx);
    secondPhaseRandomize();
  }
  else
  {
    runSecondPhaseErrorEffect();
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

/**
 * Functions to initialize and configure the timers.
 */

void onTimerGeneral(int idx, int v, int up)
{
  showButtonLeds();
  checkTransitionToSecondPhase();
  showInvaderLeds();
  showSignalLeds();
  checkClearAudioPins();
  processAudioQueue();
  checkVictory();
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
  Serial.begin(9600);
  cleanState();
  initRelays();
  initButtons();
  initTimers();
  initLeds();
  initAudio();
  showLedStartEffect();
}

void loop()
{
  automaton.run();
}
