/*
 * ui_play.cpp — Play view (Catch the Star)
 * ──────────────────────────────────────────
 * Draws game area, star, scores. Animation updates timer bar.
 */
#include "ui_play.h"
#include "ui_common.h"
#include "game_star.h"

void uiPlayDraw() {
  drawViewHeader("PLAY", COL_PINK, "A=CATCH B=BACK");

  // Game area
  gfx->drawRoundRect(8, 28, 224, 148, 6, COL_PLAY_BDR);
  gfx->fillRoundRect(10, 30, 220, 144, 5, COL_PLAY_BG);

  gfx->setTextColor(COL_PINK); gfx->setTextSize(1);
  gfx->setCursor(52, 38);       gfx->print("CATCH THE STAR!");

  // Check timeout before drawing
  starGameCheckTimeout();

  if (starGame.visible) {
    gfx->setTextColor(COL_YELLOW); gfx->setTextSize(3);
    gfx->setCursor(starGame.x, starGame.y);
    gfx->print("*");

    // Timer bar
    uint32_t rem = starGame.showUntil - millis();
    int bw = constrain((int)map((long)rem, 0L, 2200L, 0L, 200L), 0, 200);
    gfx->fillRect(20, 148, 200, 5, COL_DARK);
    uint16_t bc = (rem > 1000) ? COL_GREEN : (rem > 500 ? COL_YELLOW : COL_PINK);
    if (bw > 0) gfx->fillRect(20, 148, bw, 5, bc);

    gfx->setTextSize(1); gfx->setTextColor(COL_DIM);
    gfx->setCursor(60, 158);
    gfx->print("[A] TAP TO CATCH!");
  } else {
    gfx->setTextSize(1); gfx->setTextColor(COL_DIM);
    gfx->setCursor(68, 100);
    gfx->print("WAIT FOR IT...");
  }

  // Score panels
  gfx->drawFastHLine(0, 182, SCREEN_W, COL_DIM);

  // SCORE
  gfx->fillRoundRect(8,  188, 108, 40, 4, COL_BAR_BG);
  gfx->drawRoundRect(8,  188, 108, 40, 4, COL_PLAY_BDR);
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(18, 194);    gfx->print("SCORE");
  gfx->setTextColor(COL_PINK); gfx->setTextSize(2);
  char sbuf[8]; sprintf(sbuf, "%d", starGame.score);
  gfx->setCursor(18, 210);    gfx->print(sbuf);

  // BEST
  gfx->fillRoundRect(124, 188, 108, 40, 4, COL_BAR_BG);
  gfx->drawRoundRect(124, 188, 108, 40, 4, COL_PLAY_BDR);
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(134, 194);   gfx->print("BEST");
  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  sprintf(sbuf, "%d", starGame.bestScore);
  gfx->setCursor(134, 210);   gfx->print(sbuf);

  // Joy earned
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 236);     gfx->print("JOY +");
  gfx->setTextColor(COL_PINK);
  sprintf(sbuf, "%d", starGame.score * 2);
  gfx->setCursor(50, 236);    gfx->print(sbuf);

  gfx->setTextColor(COL_DIM);
  gfx->setCursor(42, 260);    gfx->print("B = BACK TO MAIN");
}

void uiPlayAnimate() {
  if (currentView != VIEW_PLAY) return;

  if (starGameCheckTimeout()) {
    viewDirty = true;
    return;
  }

  if (starGame.visible) {
    uint32_t rem = starGame.showUntil - millis();
    int bw = constrain((int)map((long)rem, 0L, 2200L, 0L, 200L), 0, 200);
    gfx->fillRect(20, 148, 200, 5, COL_DARK);
    uint16_t bc = (rem > 1000) ? COL_GREEN : (rem > 500 ? COL_YELLOW : COL_PINK);
    if (bw > 0) gfx->fillRect(20, 148, bw, 5, bc);
  }
}
