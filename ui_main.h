/*
 * ui_main.h — Main view (home screen)
 */
#pragma once

#include "types.h"

void uiMainDraw();          // full draw
void uiMainAnimate();       // partial redraw (pet bob, clock)
void uiMainDrawStatBars();  // stat bars only (for decay update)
void uiMainDrawActionBar(); // action bar only (for cursor change)
