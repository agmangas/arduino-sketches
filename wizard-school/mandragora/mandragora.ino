#include <Automaton.h>
#include <CircularBuffer.h>

/**
 * LDRs and state machines.
 */

const int LDR_NUM = 3;

const byte LDR_PINS[LDR_NUM] = {
    3, 4, 5};

Atm_bit ldrBits[LDR_NUM];
Atm_timer ldrSnapshotTimer;

/**
 * Buffer of LDR snapshots.
 */

const int STATE_UNCOVERED = 1;
const int STATE_COVERED = 2;
const int STATE_MIXED = 3;

typedef struct ldrSnapshot
{
    int state;
    unsigned long tstamp;
} LdrSnapshot;

const int LDR_SNAPSHOT_BUF_SIZE = 30;
const int LDR_SNAPSHOT_TIMER_MS = 200;

CircularBuffer<LdrSnapshot, LDR_SNAPSHOT_BUF_SIZE> ldrSnapBuf;

/**
 * Number of cycles for completion.
 */

const int NUM_CYCLES = 2;

/**
 * Timings.
 */

const int UNCOVER_MAX_DELAY_MS = 5000;
const int HOLD_EXPECTED_DELAY_MS = 5000;
const int HOLD_TOLERANCE_MS = 3000;
const int COVER_MAX_DELAY_MS = 5000;
const int PRINT_HISTORY_INTERVAL_MS = 4000;

/**
 * Program state.
 */

typedef struct programState
{
    bool isSolved;
    unsigned long lastPrint;
} ProgramState;

ProgramState progState = {
    .isSolved = false,
    .lastPrint = 0};

/**
 * Audio FX.
 */

const byte AUDIO_PIN_RST = 11;
const byte AUDIO_PIN_ACT = 6;
const byte AUDIO_PIN_FINAL = 7;

const int AUDIO_NUM_TRACKS = 3;

const byte AUDIO_PINS[AUDIO_NUM_TRACKS] = {
    10, 8, 2};

/**
 * Utility functions.
 */

bool isAllowedToPrint()
{
    unsigned long now = millis();

    if (now < progState.lastPrint)
    {
        progState.lastPrint = 0;
        return false;
    }
    else if ((now - progState.lastPrint) <= PRINT_HISTORY_INTERVAL_MS)
    {
        return false;
    }

    progState.lastPrint = now;

    return true;
}

/**
 * LDR history functions.
 */

void printLdrHistory()
{
    Serial.println("##########");

    for (int i = 0; i < ldrSnapBuf.size(); i++)
    {
        Serial.print("#");
        Serial.print(i);
        Serial.print(" :: ");
        printLdrState(ldrSnapBuf[i].state);

        if (i > 0)
        {
            Serial.print(" :: ");
            Serial.println(ldrSnapBuf[i].tstamp - ldrSnapBuf[i - 1].tstamp);
        }
        else
        {
            Serial.println("");
        }
    }

    Serial.println("##########");
}

bool isValidBufIndex(int idx)
{
    return idx >= 0 && idx < ldrSnapBuf.size();
}

bool isUncoveredBelowMaxDelay(int idx)
{
    if (!isValidBufIndex(idx) ||
        idx == 0 ||
        ldrSnapBuf[idx].state != STATE_UNCOVERED)
    {
        return false;
    }

    int diffMs = ldrSnapBuf[idx].tstamp - ldrSnapBuf[idx - 1].tstamp;

    return diffMs <= UNCOVER_MAX_DELAY_MS;
}

int findNextUncoveredBelowMaxDelay(int idxStart)
{
    if (!isValidBufIndex(idxStart))
    {
        return -1;
    }

    for (int i = idxStart; i < ldrSnapBuf.size(); i++)
    {
        if (isUncoveredBelowMaxDelay(i))
        {
            return i;
        }
    }

    return -1;
}

bool isHeldWithinLimits(int idxBase)
{
    int idxNext = idxBase + 1;

    if (!isValidBufIndex(idxBase) ||
        !isValidBufIndex(idxNext) ||
        ldrSnapBuf[idxBase].state != STATE_UNCOVERED ||
        ldrSnapBuf[idxNext].state == STATE_UNCOVERED)
    {
        return false;
    }

    int limitHi = HOLD_EXPECTED_DELAY_MS + HOLD_TOLERANCE_MS;
    int limitLo = HOLD_EXPECTED_DELAY_MS - HOLD_TOLERANCE_MS;
    int diffMs = ldrSnapBuf[idxNext].tstamp - ldrSnapBuf[idxBase].tstamp;
    bool isWithinLimits = (diffMs >= limitLo) && (diffMs <= limitHi);

    return isWithinLimits;
}

bool isHeldAndCoveredWithinLimits(int idxBase)
{
    int idxNext = idxBase + 1;

    if (isHeldWithinLimits(idxBase) == false ||
        ldrSnapBuf[idxNext].state == STATE_UNCOVERED)
    {
        return false;
    }

    if (ldrSnapBuf[idxNext].state == STATE_COVERED)
    {
        return true;
    }

    if (isValidBufIndex(idxNext + 1) == false ||
        ldrSnapBuf[idxNext].state != STATE_MIXED ||
        ldrSnapBuf[idxNext + 1].state != STATE_COVERED)
    {
        return false;
    }

    int diffMs = ldrSnapBuf[idxNext + 1].tstamp - ldrSnapBuf[idxNext].tstamp;
    bool isWithinLimits = diffMs <= COVER_MAX_DELAY_MS;

    return isWithinLimits;
}

bool timerOverflowInHistory()
{
    unsigned long prev = 0;
    unsigned long curr;

    for (int i = 0; i < ldrSnapBuf.size(); i++)
    {
        curr = ldrSnapBuf[i].tstamp;

        if (prev > curr)
        {
            return true;
        }

        prev = curr;
    }

    return false;
}

int findNextCycle(int idxStart)
{
    int idxCursor = idxStart;
    int idxUncovered;

    while (idxCursor < ldrSnapBuf.size())
    {
        idxUncovered = findNextUncoveredBelowMaxDelay(idxCursor);

        if (idxUncovered == -1)
        {
            return -1;
        }

        if (!isHeldAndCoveredWithinLimits(idxUncovered))
        {
            idxCursor = idxUncovered;
            idxCursor++;
            continue;
        }

        for (int i = (idxUncovered + 1); i < ldrSnapBuf.size(); i++)
        {
            if (ldrSnapBuf[i].state == STATE_COVERED)
            {
                return i;
            }
        }

        return -1;
    }

    return -1;
}

bool isValidLdrHistory()
{
    if (timerOverflowInHistory())
    {
        ldrSnapBuf.clear();
    }

    if (ldrSnapBuf.isEmpty())
    {
        return false;
    }

    bool printAllowed = isAllowedToPrint();

    if (printAllowed)
    {
        printLdrHistory();
    }

    int idxCursor = 0;
    int numCycles = 0;
    int idxCycleEnd;

    while (idxCursor < ldrSnapBuf.size())
    {
        idxCycleEnd = findNextCycle(idxCursor);

        if (idxCycleEnd == -1)
        {
            break;
        }

        if (printAllowed)
        {
            Serial.print("Cycle :: #");
            Serial.print(idxCursor);
            Serial.print(" -> #");
            Serial.println(idxCycleEnd);
        }

        numCycles++;
        idxCursor = idxCycleEnd + 1;
    }

    return numCycles >= NUM_CYCLES ? true : false;
}

bool isLdrUncovered(int idx)
{
    return ldrBits[idx].state() == 1;
}

bool isCurrentLdrStateMixed()
{
    for (int i = 1; i < LDR_NUM; i++)
    {
        if (isLdrUncovered(i) != isLdrUncovered(0))
        {
            return true;
        }
    }

    return false;
}

int getCurrentLdrState()
{
    if (isCurrentLdrStateMixed())
    {
        return STATE_MIXED;
    }
    else if (isLdrUncovered(0))
    {
        return STATE_UNCOVERED;
    }
    else
    {
        return STATE_COVERED;
    }
}

void printLdrState(int state)
{
    if (state == STATE_MIXED)
    {
        Serial.print(F("Mixed"));
    }
    else if (state == STATE_UNCOVERED)
    {
        Serial.print(F("Uncovered"));
    }
    else if (state == STATE_COVERED)
    {
        Serial.print(F("Covered"));
    }
}

void refreshLdrBits()
{
    for (int i = 0; i < LDR_NUM; i++)
    {
        if (digitalRead(LDR_PINS[i]) == LOW)
        {
            ldrBits[i].on();
        }
        else
        {
            ldrBits[i].off();
        }
    }
}

void onLdrTimer(int idx, int v, int up)
{
    refreshLdrBits();

    LdrSnapshot currSnapshot;

    currSnapshot.state = getCurrentLdrState();
    currSnapshot.tstamp = millis();

    bool shouldPush = ldrSnapBuf.isEmpty() ||
                      ldrSnapBuf.last().state != currSnapshot.state;

    if (shouldPush)
    {
        Serial.print(F("LDR state :: "));
        printLdrState(currSnapshot.state);
        Serial.println();

        ldrSnapBuf.push(currSnapshot);
    }
}

void initLdrs()
{
    for (int i = 0; i < LDR_NUM; i++)
    {
        pinMode(LDR_PINS[i], INPUT_PULLUP);
        ldrBits[i].begin();
    }

    ldrSnapshotTimer
        .begin(LDR_SNAPSHOT_TIMER_MS)
        .repeat(-1)
        .onTimer(onLdrTimer)
        .start();
}

/**
 * Audio FX functions.
 */

void playTrack(byte trackPin)
{
    if (isTrackPlaying())
    {
        return;
    }

    Serial.print(F("Playing track: "));
    Serial.println(trackPin);

    digitalWrite(trackPin, LOW);
    pinMode(trackPin, OUTPUT);
    delay(200);
    pinMode(trackPin, INPUT);
}

void waitForTrackStop()
{
    const int waitDelayMs = 50;

    while (isTrackPlaying())
    {
        delay(waitDelayMs);
    }
}

void initAudioPins()
{
    for (int i = 0; i < LDR_NUM; i++)
    {
        pinMode(AUDIO_PINS[i], INPUT);
    }

    pinMode(AUDIO_PIN_ACT, INPUT);
    pinMode(AUDIO_PIN_RST, INPUT);
}

bool isTrackPlaying()
{
    return digitalRead(AUDIO_PIN_ACT) == LOW;
}

void resetAudio()
{
    Serial.println("Audio FX reset");

    digitalWrite(AUDIO_PIN_RST, LOW);
    pinMode(AUDIO_PIN_RST, OUTPUT);
    delay(10);
    pinMode(AUDIO_PIN_RST, INPUT);

    Serial.println("Waiting for Audio FX");

    delay(1000);
}

void updateStateAndPlayAudio()
{
    if (progState.isSolved)
    {
        return;
    }

    if (isValidLdrHistory())
    {
        progState.isSolved = true;
        ldrSnapBuf.clear();
        waitForTrackStop();
        playTrack(AUDIO_PIN_FINAL);
        return;
    }

    bool shouldPlay = !isTrackPlaying() &&
                      !ldrSnapBuf.isEmpty() &&
                      ldrSnapBuf.last().state != STATE_COVERED;

    if (shouldPlay)
    {
        long randAudioIdx = random(0, AUDIO_NUM_TRACKS);
        playTrack(AUDIO_PINS[randAudioIdx]);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initLdrs();
    initAudioPins();
    resetAudio();

    Serial.println(">> Starting Mandragora program");
}

void loop()
{
    automaton.run();
    updateStateAndPlayAudio();
}
