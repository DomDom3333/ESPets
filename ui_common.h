/*
 * ui_common.h — Shared drawing helpers
 * ──────────────────────────────────────
 * Pixel pet, notifications, splash, view headers.
 * Used by all ui_*.cpp screen modules.
 *
 * Future: drawPixelPet will accept a seed parameter
 * to procedurally vary body shape / colours.
 */
#pragma once

#include "types.h"

// Notification system
void triggerNotif(const char* msg);
void drawNotification();

// Splash screen (shown once at boot)
void drawSplash();

// Pixel pet renderer
// mood: 'h'appy, 'd'own, 's'leep   large: splash size vs normal
void drawPixelPet(int cx, int cy, char mood, bool large);

// Shared UI elements
void drawViewHeader(const char* title, uint16_t titleCol,
                    const char* hint,  uint16_t hintCol = COL_DIM);
void drawBarWithBorder(int x, int y, int w, int h,
                       uint8_t val, uint16_t fillCol);
