/*
 * game_rhythm.cpp — Rhythm Tap game logic
 * ───────────────────────────────────────
 * Beat timing, scoring, accuracy calculation.
 */
#include "game_rhythm.h"
#include "input.h"

// Global game state
RhythmGameState rhythmGame = {
  .round = 0,
  .beatIndex = 0,
  .roundComplete = false,
  .roundStartTime = 0,
  .beatInterval = 1000,
  .roundScore = 0,
  .totalScore = 0,
  .bestScore = 0,
  .perfectCount = 0,
  .goodCount = 0,
  .missCount = 0,
  .lastAccuracy = 0,
  .feedbackAge = 0,
  .feedbackColor = 0,
  .lastProcessedClick = 0,
  .currentBeatProcessed = false
};

// Beat interval progression (ms between beats)
// Round 0: Easy (1000ms)
// Round 1: Medium (800ms)
// Round 2: Hard (600ms)
static const uint16_t BEAT_INTERVALS[] = {1000, 800, 600};
static const uint8_t NUM_ROUNDS = 3;
static const uint8_t BEATS_PER_ROUND = 10;

// Scoring thresholds (ms)
#define ACCURACY_PERFECT   50   // ±50ms = 10 points
#define ACCURACY_GOOD      100  // ±100ms = 5 points
#define ACCURACY_OK        150  // ±150ms = 2 points
#define SCORE_PERFECT      10
#define SCORE_GOOD         5
#define SCORE_OK           2

// Feedback animation (ticks)
#define FEEDBACK_DURATION  10   // 600ms ticks = ~6 seconds

// ══════════════════════════════════════════════════════════
//  GAME LIFECYCLE
// ══════════════════════════════════════════════════════════

void rhythmGameInit() {
  // Called at boot — initialize best score etc.
  rhythmGame.bestScore = 0;
  rhythmGameReset();
}

void rhythmGameReset() {
  // Reset for a new game (3 rounds)
  rhythmGame.round = 0;
  rhythmGame.beatIndex = 0;
  rhythmGame.roundComplete = false;
  rhythmGame.roundScore = 0;
  rhythmGame.totalScore = 0;
  rhythmGame.perfectCount = 0;
  rhythmGame.goodCount = 0;
  rhythmGame.missCount = 0;
  rhythmGame.feedbackAge = 0;
  rhythmGame.currentBeatProcessed = false;

  rhythmGameStartRound();
}

// Start a new round
static void rhythmGameStartRound() {
  if (rhythmGame.round >= NUM_ROUNDS) {
    // Game over
    rhythmGame.roundComplete = true;
    if (rhythmGame.totalScore > rhythmGame.bestScore) {
      rhythmGame.bestScore = rhythmGame.totalScore;
    }
    return;
  }

  rhythmGame.beatIndex = 0;
  rhythmGame.roundScore = 0;
  rhythmGame.perfectCount = 0;
  rhythmGame.goodCount = 0;
  rhythmGame.missCount = 0;
  rhythmGame.roundStartTime = millis();
  rhythmGame.beatInterval = BEAT_INTERVALS[rhythmGame.round];
  rhythmGame.currentBeatProcessed = false;
  rhythmGame.feedbackAge = 0;

  Serial.printf("[RHYTHM] Round %d started (interval: %dms)\n",
                rhythmGame.round + 1, rhythmGame.beatInterval);
}

// ══════════════════════════════════════════════════════════
//  BEAT TIMING & SCORING
// ══════════════════════════════════════════════════════════

int rhythmGameGetCurrentBeatTime() {
  // Returns the expected timestamp of the current beat
  // (or 0 if no beat is active)
  if (rhythmGame.roundStartTime == 0) return 0;

  uint32_t elapsed = millis() - rhythmGame.roundStartTime;
  int expectedBeatTime = (int)(rhythmGame.beatIndex * rhythmGame.beatInterval);

  // Check if current beat is still active (within window)
  if (elapsed < expectedBeatTime) {
    return 0;  // Beat hasn't started yet
  }

  if (elapsed > expectedBeatTime + 300) {
    // Beat window has closed (±150ms + 150ms grace)
    return 0;
  }

  return rhythmGame.roundStartTime + expectedBeatTime;
}

static void rhythmGameScoreTap(uint32_t clickTime, uint32_t beatTime) {
  // Score a button tap based on accuracy
  int accuracy = (int)clickTime - (int)beatTime;  // Positive = late, negative = early
  int absAccuracy = abs(accuracy);

  int points = 0;
  const char* msg = "";
  uint16_t color = 0;

  if (absAccuracy <= ACCURACY_PERFECT) {
    points = SCORE_PERFECT;
    msg = "PERFECT!";
    color = ((uint16_t)0x3FE3);  // COL_GREEN
    rhythmGame.perfectCount++;
  } else if (absAccuracy <= ACCURACY_GOOD) {
    points = SCORE_GOOD;
    msg = "GOOD";
    color = ((uint16_t)0xFF60);  // COL_YELLOW
    rhythmGame.goodCount++;
  } else if (absAccuracy <= ACCURACY_OK) {
    points = SCORE_OK;
    msg = "OK";
    color = ((uint16_t)0xFB40);  // COL_ORANGE
    rhythmGame.goodCount++;
  } else {
    msg = "MISS!";
    color = ((uint16_t)0xF800);  // Red
    rhythmGame.missCount++;
  }

  rhythmGame.roundScore += points;
  rhythmGame.totalScore += points;
  rhythmGame.lastAccuracy = accuracy;
  strncpy(rhythmGame.feedbackMsg, msg, sizeof(rhythmGame.feedbackMsg) - 1);
  rhythmGame.feedbackColor = color;
  rhythmGame.feedbackAge = FEEDBACK_DURATION;

  Serial.printf("[RHYTHM] Beat %d: %s (%+dms, +%d pts)\n",
                rhythmGame.beatIndex + 1, msg, accuracy, points);
}

// ══════════════════════════════════════════════════════════
//  GAME UPDATE
// ══════════════════════════════════════════════════════════

void rhythmGameUpdate() {
  // Called every frame
  // Check if button was pressed and score it

  if (rhythmGame.roundComplete) {
    return;  // Game over
  }

  if (rhythmGame.roundStartTime == 0) {
    return;  // Round not started
  }

  // Advance beat if enough time has passed
  uint32_t elapsed = millis() - rhythmGame.roundStartTime;
  int expectedBeatIndex = elapsed / rhythmGame.beatInterval;

  // Detect beat transitions and mark misses
  if (expectedBeatIndex > rhythmGame.beatIndex) {
    // We've moved past the current beat without a tap
    if (!rhythmGame.currentBeatProcessed) {
      // The current beat was missed
      rhythmGame.missCount++;
      rhythmGame.lastAccuracy = 500;  // Large miss
      strncpy(rhythmGame.feedbackMsg, "MISS!", sizeof(rhythmGame.feedbackMsg) - 1);
      rhythmGame.feedbackColor = ((uint16_t)0xF800);  // Red
      rhythmGame.feedbackAge = FEEDBACK_DURATION;
      Serial.printf("[RHYTHM] Beat %d: MISS\n", rhythmGame.beatIndex + 1);
    }

    rhythmGame.beatIndex = expectedBeatIndex;
    rhythmGame.currentBeatProcessed = false;

    // Check if round is complete
    if (rhythmGame.beatIndex >= BEATS_PER_ROUND) {
      rhythmGame.round++;
      if (rhythmGame.round < NUM_ROUNDS) {
        rhythmGameStartRound();
      } else {
        rhythmGame.roundComplete = true;
        if (rhythmGame.totalScore > rhythmGame.bestScore) {
          rhythmGame.bestScore = rhythmGame.totalScore;
        }
        Serial.printf("[RHYTHM] Game Complete! Score: %d\n", rhythmGame.totalScore);
      }
    }
  }

  // Check for button press (click happened since last frame)
  // Note: lastClickTime is updated by input.cpp's onClick handler
  extern uint32_t lastClickTime;

  if (lastClickTime > rhythmGame.lastProcessedClick) {
    rhythmGame.lastProcessedClick = lastClickTime;

    // User pressed the button - score it
    uint32_t currentBeatTime = rhythmGame.roundStartTime +
                               (rhythmGame.beatIndex * rhythmGame.beatInterval);

    // Calculate accuracy (how far off from beat this tap was)
    int32_t timeDelta = (int32_t)lastClickTime - (int32_t)currentBeatTime;
    int32_t absTimeDelta = abs(timeDelta);

    if (absTimeDelta <= ACCURACY_OK) {
      // Within scoring window
      rhythmGameScoreTap(lastClickTime, currentBeatTime);
      rhythmGame.currentBeatProcessed = true;
    } else {
      // Too early or too late to count
      strncpy(rhythmGame.feedbackMsg, "TOO EARLY!", sizeof(rhythmGame.feedbackMsg) - 1);
      rhythmGame.feedbackColor = ((uint16_t)0xF800);  // Red
      rhythmGame.feedbackAge = FEEDBACK_DURATION;
      Serial.printf("[RHYTHM] Too early/late: %+ldms\n", timeDelta);
    }
  }

  // Decay feedback animation
  if (rhythmGame.feedbackAge > 0) {
    rhythmGame.feedbackAge--;
  }
}

bool rhythmGameCheckRoundComplete() {
  return rhythmGame.roundComplete;
}
