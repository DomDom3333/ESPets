/*
 * ui_play_rhythm.cpp — Rhythm Tap game UI rendering
 * ──────────────────────────────────────────────────
 * Beat meter animation, feedback display, score rendering.
 */
#include "ui_play_rhythm.h"
#include "game_rhythm.h"
#include "ui_common.h"

// ══════════════════════════════════════════════════════════
//  FULL DRAW (on view entry)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmDraw() {
  drawViewHeader("RHYTHM TAP", COL_CYAN, "A=TAP TO BEAT  B=BACK");

  // ─── BIG BEAT METER (MAIN VISUAL) ──────────────────────
  // Huge, easy-to-see progress bar in middle of screen
  gfx->fillRect(8, 50, SCREEN_W - 16, 60, COL_DARK);
  gfx->drawRect(8, 50, SCREEN_W - 16, 60, COL_DIM);

  // ─── ROUND INDICATOR (TOP) ────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(12, 40);
  gfx->printf("ROUND %d/3 • BEAT %d/10", rhythmGame.round + 1, min(rhythmGame.beatIndex + 1, 10));

  // ─── DIFFICULTY DISPLAY ───────────────────────────────
  gfx->setCursor(SCREEN_W - 100, 40);
  const char* diffStr = (rhythmGame.beatInterval == 1000) ? "EASY" :
                        (rhythmGame.beatInterval == 800) ? "MEDIUM" : "HARD";
  gfx->printf("Speed: %s", diffStr);

  // ─── BEAT METER FILL ──────────────────────────────────
  if (rhythmGame.roundStartTime > 0) {
    uint32_t elapsed = millis() - rhythmGame.roundStartTime;
    int barFill = ((elapsed % rhythmGame.beatInterval) * (SCREEN_W - 20)) / rhythmGame.beatInterval;

    // Color: green -> yellow -> red as beat approaches
    uint16_t barColor;
    if (barFill < (SCREEN_W - 20) / 3) {
      barColor = COL_GREEN;
    } else if (barFill < 2 * (SCREEN_W - 20) / 3) {
      barColor = COL_YELLOW;
    } else {
      barColor = COL_PINK;
    }

    gfx->fillRect(12, 56, barFill, 48, barColor);
  }

  // ─── "TAP NOW!" TEXT OVERLAY ──────────────────────────
  gfx->setTextColor(COL_WHITE); gfx->setTextSize(3);
  gfx->setCursor(60, 75);
  gfx->print("TAP!");

  // ─── SCORE SECTION ────────────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(12, 130);
  gfx->print("SCORE:");

  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  char sbuf[12];
  sprintf(sbuf, "%d", rhythmGame.totalScore);
  gfx->setCursor(100, 125);
  gfx->print(sbuf);

  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(12, 150);
  gfx->print("BEST:");
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  sprintf(sbuf, "%d", rhythmGame.bestScore);
  gfx->setCursor(100, 150);
  gfx->print(sbuf);

  // ─── ACCURACY STATS ───────────────────────────────────
  gfx->setTextColor(COL_GREEN); gfx->setTextSize(1);
  gfx->setCursor(12, 170);
  gfx->printf("Perfect: %d", rhythmGame.perfectCount);

  gfx->setTextColor(COL_YELLOW);
  gfx->setCursor(100, 170);
  gfx->printf("Good: %d", rhythmGame.goodCount);

  gfx->setTextColor(COL_PINK);
  gfx->setCursor(12, 185);
  gfx->printf("Misses: %d", rhythmGame.missCount);

  // ─── INSTRUCTIONS ────────────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(20, 210);
  gfx->print("Watch the bar fill...");
  gfx->setCursor(20, 225);
  gfx->print("TAP [A] when it reaches");
  gfx->setCursor(20, 240);
  gfx->print("the right side!");

  gfx->setCursor(8, 260);
  gfx->print("Press [B] to go back");
}

// ══════════════════════════════════════════════════════════
//  ANIMATION UPDATE (per ~100ms during gameplay)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmAnimate() {
  // ─── BEAT METER UPDATE ────────────────────────────────
  gfx->fillRect(12, 56, SCREEN_W - 24, 48, COL_DARK);

  if (rhythmGame.roundStartTime > 0) {
    uint32_t elapsed = millis() - rhythmGame.roundStartTime;
    int barFill = ((elapsed % rhythmGame.beatInterval) * (SCREEN_W - 20)) / rhythmGame.beatInterval;

    uint16_t barColor;
    if (barFill < (SCREEN_W - 20) / 3) {
      barColor = COL_GREEN;
    } else if (barFill < 2 * (SCREEN_W - 20) / 3) {
      barColor = COL_YELLOW;
    } else {
      barColor = COL_PINK;
    }

    gfx->fillRect(12, 56, barFill, 48, barColor);
  }

  gfx->drawRect(8, 50, SCREEN_W - 16, 60, COL_DIM);

  // ─── SCORE UPDATE ────────────────────────────────────
  gfx->fillRect(100, 120, 80, 20, COL_BG_PLAY);
  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  char sbuf[12];
  sprintf(sbuf, "%d", rhythmGame.totalScore);
  gfx->setCursor(100, 132);
  gfx->print(sbuf);

  // ─── COUNTERS UPDATE ──────────────────────────────────
  gfx->fillRect(12, 165, SCREEN_W - 24, 20, COL_BG_PLAY);
  gfx->setTextColor(COL_GREEN); gfx->setTextSize(1);
  gfx->setCursor(12, 173);
  gfx->printf("Perfect: %d", rhythmGame.perfectCount);
  gfx->setTextColor(COL_YELLOW);
  gfx->setCursor(100, 173);
  gfx->printf("Good: %d", rhythmGame.goodCount);
  gfx->setTextColor(COL_PINK);
  gfx->setCursor(12, 187);
  gfx->printf("Misses: %d", rhythmGame.missCount);

  // ─── FEEDBACK FLASH ───────────────────────────────────
  if (rhythmGame.feedbackAge > 0) {
    // Large, prominent feedback in center
    gfx->fillRect(30, 30, SCREEN_W - 60, 25, rhythmGame.feedbackColor);
    gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
    gfx->setCursor(40, 42);
    gfx->print(rhythmGame.feedbackMsg);
    rhythmGame.feedbackAge--;  // Decay locally for animation
  }

  // ─── GAME OVER CHECK ──────────────────────────────────
  static uint32_t gameOverTime = 0;
  if (rhythmGame.roundComplete && gameOverTime == 0) {
    gameOverTime = millis();
  }

  if (gameOverTime > 0 && millis() - gameOverTime > 3000) {
    // Auto-return to main after 3 seconds
    navSwitchView(VIEW_MAIN);
    gameOverTime = 0;
  }

  if (rhythmGame.roundComplete) {
    gfx->fillRect(20, 120, SCREEN_W - 40, 60, COL_GREEN);
    gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
    gfx->setCursor(35, 135);
    gfx->print("GAME OVER!");
    gfx->setTextSize(1);
    gfx->setCursor(30, 155);
    char buf[32];
    sprintf(buf, "Final: %d  Best: %d", rhythmGame.totalScore, rhythmGame.bestScore);
    gfx->print(buf);
    gfx->setCursor(40, 170);
    gfx->print("Returning...");
  }
}
