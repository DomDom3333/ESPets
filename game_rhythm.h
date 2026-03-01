/*
 * game_rhythm.h — "Rhythm Tap" mini-game
 * ──────────────────────────────────────
 * Button timing game: tap to the beat.
 * Pure game logic — no drawing.
 */
#pragma once

#include "types.h"

// Game state structure
struct RhythmGameState {
  // Game flow
  int   round            = 0;       // 0-2 (3 rounds, increasing difficulty)
  int   beatIndex        = 0;       // 0-9 (10 beats per round)
  bool  roundComplete    = false;

  // Timing
  uint32_t roundStartTime = 0;
  uint32_t beatInterval   = 1000;   // ms between beats (1000/800/600)

  // Scoring
  int   roundScore       = 0;       // Points this round
  int   totalScore       = 0;       // Points across all rounds
  int   bestScore        = 0;       // Best ever
  int   perfectCount     = 0;       // Number of perfect hits
  int   goodCount        = 0;       // Number of good hits
  int   missCount        = 0;       // Number of misses

  // Feedback
  int   lastAccuracy     = 0;       // -150 to +150 (delta from perfect in ms)
  char  feedbackMsg[20]  = "";      // "PERFECT!", "GOOD", "MISS!"
  uint8_t feedbackAge    = 0;       // Animation frames remaining for feedback
  uint16_t feedbackColor = 0;       // RGB565 color for feedback flash

  // Click tracking
  uint32_t lastProcessedClick = 0;
  bool     currentBeatProcessed = false;
};

extern RhythmGameState rhythmGame;

// Game lifecycle
void rhythmGameInit();              // Called once at boot (resets best score state)
void rhythmGameReset();             // Reset to round 1
void rhythmGameUpdate();            // Called every frame to check beat timing
bool rhythmGameCheckRoundComplete();  // Returns true if round/game is finished
int  rhythmGameGetCurrentBeatTime(); // Returns expected time of current beat
