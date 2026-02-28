/*
 * game_star.cpp — "Catch the Star" mini-game logic
 * ──────────────────────────────────────────────────
 */
#include "game_star.h"
#include "ui_common.h"  // triggerNotif

void starGameReset() {
  starGame.x         = (int)random(30, 190);
  starGame.y         = (int)random(50, 150);
  starGame.showUntil = millis() + (uint32_t)max(600, 2200 - starGame.score * 80);
  starGame.visible   = true;
}

void starGameCatch() {
  if (!starGame.visible) {
    triggerNotif("WAIT FOR THE STAR!");
    return;
  }
  starGame.score++;
  if (starGame.score > starGame.bestScore)
    starGame.bestScore = starGame.score;

  pet.happy  = (uint8_t)min(100, (int)pet.happy  + 3);
  pet.energy = (uint8_t)max(0,   (int)pet.energy - 1);

  char msg[28];
  sprintf(msg, "CAUGHT!  SCORE: %d", starGame.score);
  triggerNotif(msg);

  starGame.visible = false;
  starGameReset();
}

bool starGameCheckTimeout() {
  if (starGame.visible && millis() >= starGame.showUntil) {
    starGame.visible = false;
    starGame.score   = max(0, starGame.score - 1);
    starGameReset();
    return true;   // expired — caller should redraw
  }
  return false;
}
