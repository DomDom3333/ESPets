/*
 * input.h — Single-button input via OneButton library
 * ─────────────────────────────────────────────────────
 * All navigation through one button:
 *   single click  → cycle / next / catch   (navOnShortPressA)
 *   double click  → select / action / back  (navOnShortPressB)
 *   long press    → toggle sleep             (navOnLongPressA)
 *
 * Requires: OneButton library (by Matthias Hertel)
 *           Install via Arduino Library Manager.
 */
#pragma once

#include "types.h"

// Call once in setup()
void inputInit();

// Call every loop() iteration
void inputUpdate();

// ── Click timing (exported for games like Rhythm Tap) ────
// Updated in onClick() handler; games read this to detect taps
extern uint32_t lastClickTime;
