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
static void onClick() {
  Serial.println("DEBUG: Single click detected");
  navOnShortPressA();
}
static void onDoubleClick() {
  Serial.println("DEBUG: Double click detected");
  navOnShortPressB();
}
static void onLongPress() {
  Serial.println("DEBUG: Long press detected");
  navOnLongPressA();
}

void inputInit() {
  // Drain any boot-press so it isn't misread as a click
  pinMode(BTN_PIN, INPUT_PULLUP);
  Serial.println("DEBUG: Button pin configured as INPUT_PULLUP");
  delay(200);
  while (digitalRead(BTN_PIN) == LOW) delay(10);
  Serial.println("DEBUG: Boot-press drained");

  // Now create OneButton — GPIO subsystem is ready
  btn = new OneButton(BTN_PIN, true, true);
  btn->attachClick(onClick);
  btn->attachDoubleClick(onDoubleClick);
  btn->attachLongPressStart(onLongPress);
  Serial.println("DEBUG: OneButton created and callbacks attached");
}

void inputUpdate() {
  static uint32_t lastDebugTime = 0;
  uint32_t now = millis();

  // Log raw button state every 500ms
  if (now - lastDebugTime >= 500) {
    lastDebugTime = now;
    int buttonState = digitalRead(BTN_PIN);
    Serial.printf("DEBUG: Button state: %d (0=LOW/pressed, 1=HIGH/released)\n", buttonState);
  }

  btn->tick();
}
