/*
 * ui_play_rhythm.cpp — Rhythm Tap game UI rendering
 * ──────────────────────────────────────────────────
 * Beat meter animation, feedback display, score rendering.
 */
#include "ui_play_rhythm.h"
#include "game_rhythm.h"
#include "ui_common.h"

// ══════════════════════════════════════════════════════════
//  HELPERS
// ══════════════════════════════════════════════════════════

static void drawBeatMeter() {
  // Beat meter: fills left-to-right showing progress to next beat
  if (rhythmGame.roundStartTime == 0) return;

  uint32_t elapsed = millis() - rhythmGame.roundStartTime;
  int barFill = (elapsed % rhythmGame.beatInterval) * 200 / rhythmGame.beatInterval;

  gfx->fillRect(20, 48, 200, 8, COL_DARK);
  if (barFill > 0) {
    // Color shifts from green to red as beat approaches
    uint16_t barColor = (barFill > 160) ? COL_PINK : (barFill > 80 ? COL_YELLOW : COL_GREEN);
    gfx->fillRect(20, 48, barFill, 8, barColor);
  }
  gfx->drawRect(20, 48, 200, 8, COL_DIM);
}

// ══════════════════════════════════════════════════════════
//  FULL DRAW (on view entry)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmDraw() {
  drawViewHeader("RHYTHM TAP", COL_CYAN, "A=TAP  B=BACK");

  // ─── BEAT LABEL ───────────────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(20, 42);
  gfx->print("BEAT:");

  // ─── BEAT METER (bar) ─────────────────────────────────
  drawBeatMeter();

  // ─── ROUND / BEAT COUNTERS ────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 68);
  gfx->printf("Round %d/3", rhythmGame.round + 1);
  gfx->setCursor(140, 68);
  gfx->printf("Beat %d/10", min(rhythmGame.beatIndex + 1, 10));

  // ─── SCORE ────────────────────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 88);  gfx->print("SCORE");
  gfx->setCursor(8, 98);  gfx->print("BEST");

  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  char sbuf[8];
  sprintf(sbuf, "%d", rhythmGame.totalScore);
  gfx->setCursor(70, 88); gfx->print(sbuf);

  gfx->setTextColor(COL_DIM); gfx->setTextSize(2);
  sprintf(sbuf, "%d", rhythmGame.bestScore);
  gfx->setCursor(70, 104); gfx->print(sbuf);

  // ─── STATS ────────────────────────────────────────────
  gfx->setTextColor(COL_GREEN); gfx->setTextSize(1);
  gfx->setCursor(8, 130);
  gfx->printf("P:%d", rhythmGame.perfectCount);
  gfx->setTextColor(COL_YELLOW);
  gfx->setCursor(60, 130);
  gfx->printf("G:%d", rhythmGame.goodCount);
  gfx->setTextColor(COL_PINK);
  gfx->setCursor(110, 130);
  gfx->printf("M:%d", rhythmGame.missCount);

  // ─── DIFFICULTY ───────────────────────────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 148);
  const char* diffStr = (rhythmGame.beatInterval == 1000) ? "EASY" :
                        (rhythmGame.beatInterval == 800)  ? "MEDIUM" : "HARD";
  gfx->printf("Speed: %s (%ldms)", diffStr, (long)rhythmGame.beatInterval);

  // Separator
  gfx->drawFastHLine(0, 162, SCREEN_W, COL_DIM);

  // ─── FEEDBACK AREA (blank initially) ──────────────────
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 175);
  gfx->print("Tap in sync with the bar!");

  gfx->setCursor(8, 260);
  gfx->print("B = BACK TO MAIN");
}

// ══════════════════════════════════════════════════════════
//  ANIMATION UPDATE (per 600ms tick)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmAnimate() {
  // ─── BEAT METER ───────────────────────────────────────
  drawBeatMeter();

  // ─── SCORE ────────────────────────────────────────────
  gfx->fillRect(70, 86, 100, 14, COL_BG_PLAY);
  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  char sbuf[8];
  sprintf(sbuf, "%d", rhythmGame.totalScore);
  gfx->setCursor(70, 98); gfx->print(sbuf);

  // ─── COUNTERS ─────────────────────────────────────────
  gfx->fillRect(8, 125, SCREEN_W - 16, 10, COL_BG_PLAY);
  gfx->setTextColor(COL_GREEN); gfx->setTextSize(1);
  gfx->setCursor(8, 133);
  gfx->printf("P:%d", rhythmGame.perfectCount);
  gfx->setTextColor(COL_YELLOW);
  gfx->setCursor(60, 133);
  gfx->printf("G:%d", rhythmGame.goodCount);
  gfx->setTextColor(COL_PINK);
  gfx->setCursor(110, 133);
  gfx->printf("M:%d", rhythmGame.missCount);

  // ─── BEAT/ROUND COUNTERS ──────────────────────────────
  gfx->fillRect(8, 62, SCREEN_W - 16, 10, COL_BG_PLAY);
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, 70);
  gfx->printf("Round %d/3", rhythmGame.round + 1);
  gfx->setCursor(140, 70);
  gfx->printf("Beat %d/10", min(rhythmGame.beatIndex + 1, 10));

  // ─── FEEDBACK FLASH ───────────────────────────────────
  if (rhythmGame.feedbackAge > 0) {
    gfx->fillRect(8, 168, SCREEN_W - 16, 20, rhythmGame.feedbackColor);
    gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
    gfx->setCursor(50, 183);
    gfx->print(rhythmGame.feedbackMsg);
  } else {
    gfx->fillRect(8, 168, SCREEN_W - 16, 20, COL_BG_PLAY);
  }

  // ─── GAME OVER OVERLAY ────────────────────────────────
  if (rhythmGame.roundComplete) {
    gfx->fillRect(20, 200, SCREEN_W - 40, 40, COL_DIM);
    gfx->setTextColor(COL_YELLOW); gfx->setTextSize(1);
    gfx->setCursor(40, 214);
    gfx->print("GAME OVER!");
    gfx->setCursor(30, 228);
    char buf[24]; sprintf(buf, "Final: %d  Best: %d", rhythmGame.totalScore, rhythmGame.bestScore);
    gfx->print(buf);
  }
}
