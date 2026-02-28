/*
 * game_star.h — "Catch the Star" mini-game interface
 * ────────────────────────────────────────────────────
 * Pure game logic — no drawing.
 * Future: add more mini-games in separate game_*.h/.cpp pairs.
 */
#pragma once

#include "types.h"

void starGameReset();          // new star position + timer
void starGameCatch();          // player pressed catch
bool starGameCheckTimeout();   // returns true if star expired
