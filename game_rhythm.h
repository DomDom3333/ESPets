/*
 * game_rhythm.h — "Rhythm Tap" mini-game
 * ──────────────────────────────────────
 * Button timing game: tap to the beat.
 * Pure game logic — no drawing.
 */
#pragma once

#include "types.h"

// RhythmGameState struct is defined in types.h
extern RhythmGameState rhythmGame;

// Game lifecycle
void rhythmGameInit();              // Called once at boot (resets best score state)
void rhythmGameReset();             // Reset to round 1
void rhythmGameUpdate();            // Called every frame to check beat timing
bool rhythmGameCheckRoundComplete();  // Returns true if round/game is finished
int  rhythmGameGetCurrentBeatTime(); // Returns expected time of current beat
