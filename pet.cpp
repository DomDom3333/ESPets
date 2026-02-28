/*
 * pet.cpp — Pet logic implementation
 * ────────────────────────────────────
 * All stat manipulation lives here.
 * Returns notification strings via triggerNotif (from ui_common).
 */
#include "pet.h"
#include "ui_common.h"   // triggerNotif

// ── Stat decay / recovery ─────────────────────────────────
void petTickDecay() {
  if (!pet.sleeping) {
    pet.hunger = (uint8_t)max(0, (int)pet.hunger - 2);
    pet.happy  = (uint8_t)max(0, (int)pet.happy  - 1);
    pet.energy = (uint8_t)max(0, (int)pet.energy - 1);
    if (pet.hunger < 20)
      pet.hp = (uint8_t)max(0, (int)pet.hp - 1);
  } else {
    pet.energy = (uint8_t)min(100, (int)pet.energy + 5);
    pet.hp     = (uint8_t)min(100, (int)pet.hp     + 2);
  }

  // Trigger warnings
  if (pet.hunger < 15)
    triggerNotif("BYTE IS HUNGRY!");
  else if (pet.energy < 10 && !pet.sleeping)
    triggerNotif("BYTE IS TIRED!");
}

// ── Feeding ───────────────────────────────────────────────
void petFeed(int foodIndex) {
  if (foodIndex < 0 || foodIndex >= 6) return;

  uint8_t pts = foods[foodIndex].pts;
  pet.hunger = (uint8_t)min(100, (int)pet.hunger + pts);
  pet.happy  = (uint8_t)min(100, (int)pet.happy  + pts / 5);
  pet.weight = (uint8_t)min(99,  (int)pet.weight + 1);

  char msg[36];
  sprintf(msg, "YUMMY! %s +%d!", foods[foodIndex].name, pts);
  triggerNotif(msg);
}

// ── Mood queries ──────────────────────────────────────────
char petGetMood() {
  if (pet.sleeping)    return 's';
  if (pet.hunger < 20) return 'd';
  if (pet.happy  < 30) return 'd';
  if (pet.energy < 15) return 's';
  return 'h';
}

const char* petGetMoodString() {
  if (pet.sleeping)    return "SLEEPING";
  if (pet.hunger < 20) return "HUNGRY!";
  if (pet.happy  < 30) return "BORED...";
  if (pet.energy < 15) return "TIRED...";
  if (pet.happy  > 80) return "JOYFUL!";
  return "HAPPY :)";
}

// ── Sleep ─────────────────────────────────────────────────
void petSetSleeping(bool sleep) {
  pet.sleeping = sleep;
}
