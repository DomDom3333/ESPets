/*
 * ui_play_balance.h — Tilt Maze game UI
 * ────────────────────────────────────
 * Maze rendering, ball display, and game state visualization.
 */
#pragma once

#include "types.h"

// Full screen draw (when entering balance game)
void uiPlayBalanceDraw();

// Partial animation update (called every frame/tick during gameplay)
void uiPlayBalanceAnimate();
