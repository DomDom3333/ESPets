/*
 * input.cpp — Button input handling
 * ───────────────────────────────────
 * Pure input detection — no game/UI logic.
 * Dispatches to nav.h callbacks.
 */
#include "input.h"
#include "nav.h"

// ── Internal state ────────────────────────────────────────
static bool     sAWasDown   = false;
static bool     sBWasDown   = false;
static uint32_t sADownAt    = 0;
static bool     sLongUsed   = false;
static uint32_t sLastBTime  = 0;

void inputInit() {
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  delay(200);
  // Drain any boot-press on BTN_A
  while (digitalRead(BTN_A) == LOW) delay(10);
  sADownAt = millis();
}

void inputUpdate(uint32_t now) {
  bool aDown = (digitalRead(BTN_A) == LOW);
  bool bDown = (digitalRead(BTN_B) == LOW);

  // ── Button A ────────────────────────────────────────────
  if (aDown && !sAWasDown) {
    sADownAt  = now;
    sLongUsed = false;
  }
  if (aDown && sAWasDown && !sLongUsed) {
    if (now - sADownAt >= LONG_PRESS_MS) {
      sLongUsed = true;
      navOnLongPressA();
    }
  }
  if (!aDown && sAWasDown) {
    if (!sLongUsed && (now - sADownAt) >= DEBOUNCE_MS) {
      navOnShortPressA();
    }
  }
  sAWasDown = aDown;

  // ── Button B ────────────────────────────────────────────
  if (!bDown && sBWasDown) {
    if ((now - sLastBTime) >= DEBOUNCE_MS) {
      sLastBTime = now;
      navOnShortPressB();
    }
  }
  sBWasDown = bDown;
}
