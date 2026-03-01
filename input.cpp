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

// Pointer — constructed in inputInit() so GPIO is ready.
// (ESP32 GPIO hardware is not initialised during static
//  construction, which runs before setup().)
static OneButton* btn = nullptr;

// ── Callbacks ────────────────────────────────────────────
static void onClick()      { navOnShortPressA(); }
static void onDoubleClick(){ navOnShortPressB(); }
static void onLongPress()  { navOnLongPressA();  }

void inputInit() {
  // Drain any boot-press so it isn't misread as a click
  pinMode(BTN_PIN, INPUT_PULLUP);
  delay(200);
  while (digitalRead(BTN_PIN) == LOW) delay(10);

  // Now create OneButton — GPIO subsystem is ready
  btn = new OneButton(BTN_PIN, true, true);
  btn->attachClick(onClick);
  btn->attachDoubleClick(onDoubleClick);
  btn->attachLongPressStart(onLongPress);
}

void inputUpdate() {
  btn->tick();
}
