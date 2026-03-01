/*
 * ui_play_rhythm.h — Rhythm Tap game UI
 * ──────────────────────────────────────
 * Rendering for beat meter, feedback, and score display.
 */
#pragma once

#include "types.h"

// Full screen draw (when entering rhythm game)
void uiPlayRhythmDraw();

// Partial animation update (called every 600ms tick during gameplay)
void uiPlayRhythmAnimate();
