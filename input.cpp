/*
 * input.cpp — Single-button input via OneButton library
 * ──────────────────────────────────────────────────────
 * Replaces the manual two-button debounce with OneButton,
 * mapping click / double-click / long-press to the three
 * navigation callbacks in nav.h.
 */
#include "input.h"
#include "nav.h"
#include <OneButton.h>

// ── OneButton instance (active LOW, internal pull-up) ────
static OneButton btn(BTN_PIN, true, true);

// ── Callbacks ────────────────────────────────────────────
static void onClick()      { navOnShortPressA(); }
static void onDoubleClick(){ navOnShortPressB(); }
static void onLongPress()  { navOnLongPressA();  }

void inputInit() {
  btn.attachClick(onClick);
  btn.attachDoubleClick(onDoubleClick);
  btn.attachLongPressStart(onLongPress);

  // Drain any boot-press so it isn't misread as a click
  delay(200);
  while (digitalRead(BTN_PIN) == LOW) delay(10);
}

void inputUpdate() {
  btn.tick();
}
