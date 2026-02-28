/*
 * pet.h — Pet logic interface
 * ────────────────────────────
 * Pure state manipulation — no drawing.
 * Future: evolution checks, seed-based trait modifiers.
 */
#pragma once

#include "types.h"

// Stat decay / recovery (called on timer)
void        petTickDecay();

// Feeding
void        petFeed(int foodIndex);

// Mood queries
char        petGetMood();          // 'h'appy 'd'own 's'leep
const char* petGetMoodString();    // "HAPPY :)" etc.

// Sleep toggle
void        petSetSleeping(bool sleep);

// Apply creature DNA stat modifiers to initial pet state
void        petApplyDNA();
