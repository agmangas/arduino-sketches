#include <Arduino.h>
#include <Automaton.h>
#include <CircularBuffer.h>

/**
 * Buttons.
 */

const uint8_t BUTTONS_NUM = 6;
const uint8_t BUTTONS_PINS[BUTTONS_NUM] = {A0, A1, A2, A3, A4, A5};
Atm_button buttons[BUTTONS_NUM];

const uint8_t BUTTON_COMBINATION_VICTORY_SIZE = 6;

// 0, 2, 2, 1, 4, 5

const uint8_t BUTTON_COMBINATION_VICTORY_KEY[BUTTON_COMBINATION_VICTORY_SIZE] = {
    0, 2, 2, 1, 4, 5};

const int BUTTONS_DEBOUNCE_MS = 50;

/**
 * Audio FX.
 */

const uint8_t PIN_AUDIO_ACT = 2;
const uint8_t PIN_AUDIO_RST = 3;
const uint8_t PIN_AUDIO_TRACK_BACKGROUND = 4;
const uint8_t PIN_AUDIO_TRACK_VICTORY = 5;
const uint8_t PIN_AUDIO_TRACK_CLEANER = 6;
const unsigned long AUDIO_PLAY_DELAY_MS = 200;

/**
 * Relays.
 */

const int PIN_RELAY_ONE = 10;
const int PIN_RELAY_TWO = 11;

/**
 * Program state.
 */

CircularBuffer<uint8_t, BUTTON_COMBINATION_VICTORY_SIZE> buttonBuf;

const uint8_t AUDIO_BUF_SIZE = 2;
CircularBuffer<uint8_t, AUDIO_BUF_SIZE> audioPinsQueue;

const uint32_t TIMER_GENERAL_MS = 150;
Atm_timer timerGeneral;

typedef struct programState
{
  unsigned long audioPlayMillis;
} ProgramState;

ProgramState progState = {
    .audioPlayMillis = 0};

void cleanState()
{
  progState.audioPlayMillis = 0;
  buttonBuf.clear();
  audioPinsQueue.clear();
}

/**
 * Functions to manage the relay.
 */

void lockRelays()
{
  digitalWrite(PIN_RELAY_ONE, LOW);
  digitalWrite(PIN_RELAY_TWO, LOW);
}

void openRelays()
{
  digitalWrite(PIN_RELAY_ONE, HIGH);
  digitalWrite(PIN_RELAY_TWO, HIGH);
}

void initRelays()
{
  pinMode(PIN_RELAY_ONE, OUTPUT);
  pinMode(PIN_RELAY_TWO, OUTPUT);
  lockRelays();
}

/**
 * Functions to configure and control the audio FX.
 */

void initAudioPins()
{
  pinMode(PIN_AUDIO_TRACK_BACKGROUND, INPUT);
  pinMode(PIN_AUDIO_TRACK_VICTORY, INPUT);
  pinMode(PIN_AUDIO_TRACK_CLEANER, INPUT);
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
  pinMode(PIN_AUDIO_TRACK_BACKGROUND, INPUT);
  pinMode(PIN_AUDIO_TRACK_VICTORY, INPUT);
  pinMode(PIN_AUDIO_TRACK_CLEANER, INPUT);
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
 * Function to check whether the button combination is correct.
 */

bool isButtonCombinationCorrect()
{
  if (buttonBuf.size() != BUTTON_COMBINATION_VICTORY_SIZE)
  {
    return false;
  }

  for (uint8_t idx = 0; idx < BUTTON_COMBINATION_VICTORY_SIZE; idx++)
  {
    if (buttonBuf[idx] != BUTTON_COMBINATION_VICTORY_KEY[idx])
    {
      return false;
    }
  }

  return true;
}

/**
 * Function to run when the button combination is correct.
 */

void onButtonCombinationCorrect()
{
  const unsigned long iterDelayMs = 100;
  Serial.println(F("Button combination correct"));
  clearAudioPins();
  playTrack(PIN_AUDIO_TRACK_VICTORY, false);

  while (isTrackPlaying())
  {
    delay(iterDelayMs);
  }

  openRelays();
  playTrack(PIN_AUDIO_TRACK_CLEANER, false);

  while (isTrackPlaying())
  {
    delay(iterDelayMs);
  }
}

/**
 * Functions to handle button presses.
 */

void onPress(int idxButton, int v, int up)
{
  Serial.print(F("Press: "));
  Serial.println(idxButton);
  buttonBuf.push(idxButton);
}

void initButtons()
{
  for (uint8_t idxButton = 0; idxButton < BUTTONS_NUM; idxButton++)
  {
    buttons[idxButton]
        .begin(BUTTONS_PINS[idxButton])
        .debounce(BUTTONS_DEBOUNCE_MS)
        .onPress(onPress, idxButton);
  }
}

/**
 * Functions to initialize and configure timers.
 */

void onTimerGeneral(int idx, int v, int up)
{
  checkClearAudioPins();
  processAudioQueue();

  if (isButtonCombinationCorrect())
  {
    onButtonCombinationCorrect();
    cleanState();
  }
}

void initTimer()
{
  timerGeneral
      .begin(TIMER_GENERAL_MS)
      .repeat(-1)
      .onTimer(onTimerGeneral)
      .start();
}

void setup()
{
  Serial.begin(9600);
  cleanState();
  initRelays();
  initAudio();
  initButtons();
  initTimer();
  Serial.println(F("Ready"));
}

void loop()
{
  automaton.run();
}
