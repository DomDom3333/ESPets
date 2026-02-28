/*
 * nav.h — Navigation & view dispatch
 * ─────────────────────────────────────
 * Owns view transitions, full/partial redraws,
 * and button-action routing per view.
 */
#pragma once

#include "types.h"

// View management
void     navSwitchView(View v);
void     navDrawFullView();       // full redraw (on view switch)
void     navUpdateAnimation();    // partial redraw (per tick)
uint16_t navViewBgColor();        // bg colour for current view

// Button action callbacks (called from input.cpp)
void     navOnShortPressA();
void     navOnShortPressB();
void     navOnLongPressA();
