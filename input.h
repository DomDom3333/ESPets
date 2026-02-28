/*
 * input.h — Button input handling
 * ─────────────────────────────────
 * Polls hardware buttons, debounces, detects short/long press.
 * Fires callbacks defined in nav.cpp.
 */
#pragma once

#include "types.h"

// Call once in setup()
void inputInit();

// Call every loop() iteration
void inputUpdate(uint32_t now);
