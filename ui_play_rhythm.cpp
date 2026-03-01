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

// Draw a horizontal bar with a filled portion (0-100%)
static void drawProgressBar(int x, int y, int w, int h,
                           int fillPercent, uint16_t barColor, uint16_t bgColor) {
  int fillWidth = (w * fillPercent) / 100;
  gfx->fillRect(x, y, w, h, bgColor);
  if (fillWidth > 0) {
    gfx->fillRect(x, y, fillWidth, h, barColor);
  }
  gfx->drawRect(x, y, w, h, barColor);  // Border
}

// ══════════════════════════════════════════════════════════
//  FULL DRAW (on view entry)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmDraw() {
  // Full screen redraw
  gfx->fillScreen(((uint16_t)0x1013));  // COL_BG_PLAY (dark pink)

  // ─── HEADER ───────────────────────────────────────────────
  gfx->fillRect(0, 0, SCREEN_W, 24, ((uint16_t)0x1082));  // COL_DARK
  gfx->setTextColor(((uint16_t)0x07FF), ((uint16_t)0x1082));  // COL_CYAN text
  gfx->setFont(&FreeSans9pt7b);
  gfx->setCursor(8, 18);
  gfx->println("RHYTHM TAP");

  // ─── STATS BAR ────────────────────────────────────────────
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0x1013));  // White on dark

  // Round indicator (top right)
  gfx->setCursor(SCREEN_W - 70, 18);
  gfx->printf("R%d/3", rhythmGame.round + 1);

  // Beat counter (centered)
  gfx->setCursor(SCREEN_W / 2 - 20, 18);
  gfx->printf("B %d/10", rhythmGame.beatIndex + 1);

  // ─── BEAT METER (large, centered) ─────────────────────────
  gfx->fillRect(0, 40, SCREEN_W, 20, ((uint16_t)0x0811));  // Game area bg
  drawProgressBar(20, 48, SCREEN_W - 40, 10, 0, ((uint16_t)0x07FF), ((uint16_t)0x0811));

  // ─── SCORE DISPLAY ────────────────────────────────────────
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(((uint16_t)0xFF60), ((uint16_t)0x1013));  // COL_YELLOW
  gfx->setCursor(SCREEN_W / 2 - 30, 100);
  gfx->printf("SCORE\n%d", rhythmGame.totalScore);

  // ─── INSTRUCTION TEXT ─────────────────────────────────────
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0x1013));
  gfx->setCursor(8, 130);
  gfx->println("[A] TAP TO BEAT!");

  // ─── STATS ────────────────────────────────────────────────
  gfx->setFont(&FreeSans7pt7b);
  gfx->setCursor(8, 160);
  gfx->printf("Perfect: %d   Good: %d   Miss: %d",
             rhythmGame.perfectCount, rhythmGame.goodCount, rhythmGame.missCount);

  // ─── DIFFICULTY HINT ──────────────────────────────────────
  gfx->setCursor(8, 180);
  const char* diffStr = (rhythmGame.beatInterval == 1000) ? "EASY" :
                        (rhythmGame.beatInterval == 800) ? "MEDIUM" : "HARD";
  gfx->printf("Difficulty: %s (%dms/beat)", diffStr, rhythmGame.beatInterval);

  // ─── BEST SCORE ───────────────────────────────────────────
  gfx->setCursor(8, 200);
  gfx->printf("Best: %d", rhythmGame.bestScore);

  // Initial beat meter state
  uint32_t elapsed = millis() - rhythmGame.roundStartTime;
  int beatPercent = (elapsed % rhythmGame.beatInterval) * 100 / rhythmGame.beatInterval;
  drawProgressBar(20, 48, SCREEN_W - 40, 10, beatPercent, ((uint16_t)0x07FF), ((uint16_t)0x0811));
}

// ══════════════════════════════════════════════════════════
//  ANIMATION UPDATE (per 600ms tick)
// ══════════════════════════════════════════════════════════

void uiPlayRhythmAnimate() {
  // Called every 600ms tick during gameplay
  // Update beat meter progress and feedback display

  // ─── UPDATE BEAT METER ─────────────────────────────────────
  if (rhythmGame.roundStartTime > 0) {
    uint32_t elapsed = millis() - rhythmGame.roundStartTime;
    int beatPercent = (elapsed % rhythmGame.beatInterval) * 100 / rhythmGame.beatInterval;

    // Redraw beat meter area
    gfx->fillRect(20, 48, SCREEN_W - 40, 10, ((uint16_t)0x0811));
    drawProgressBar(20, 48, SCREEN_W - 40, 10, beatPercent, ((uint16_t)0x07FF), ((uint16_t)0x0811));
  }

  // ─── UPDATE SCORE ─────────────────────────────────────────
  gfx->fillRect(SCREEN_W / 2 - 40, 85, 80, 25, ((uint16_t)0x1013));
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(((uint16_t)0xFF60), ((uint16_t)0x1013));
  gfx->setCursor(SCREEN_W / 2 - 25, 100);
  gfx->printf("%d", rhythmGame.totalScore);

  // ─── UPDATE BEAT COUNTER ──────────────────────────────────
  gfx->fillRect(SCREEN_W / 2 - 10, 8, 40, 16, ((uint16_t)0x1082));
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(((uint16_t)0x07FF), ((uint16_t)0x1082));
  gfx->setCursor(SCREEN_W / 2 - 8, 18);
  gfx->printf("B%d/10", rhythmGame.beatIndex + 1);

  // ─── FEEDBACK ANIMATION ───────────────────────────────────
  if (rhythmGame.feedbackAge > 0) {
    // Draw feedback message with fade
    uint8_t alpha = (rhythmGame.feedbackAge * 255) / 10;  // Fade out
    gfx->setFont(&FreeSans12pt7b);

    // Background for feedback
    gfx->fillRect(40, 115, SCREEN_W - 80, 30, rhythmGame.feedbackColor);
    gfx->setTextColor(((uint16_t)0xFFFF), rhythmGame.feedbackColor);
    gfx->setCursor(SCREEN_W / 2 - 40, 140);
    gfx->println(rhythmGame.feedbackMsg);
  } else {
    // Clear feedback area
    gfx->fillRect(40, 115, SCREEN_W - 80, 30, ((uint16_t)0x1013));
  }

  // ─── UPDATE STATS ─────────────────────────────────────────
  gfx->fillRect(8, 155, SCREEN_W - 16, 16, ((uint16_t)0x1013));
  gfx->setFont(&FreeSans7pt7b);
  gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0x1013));
  gfx->setCursor(8, 165);
  gfx->printf("Perfect: %d   Good: %d   Miss: %d",
             rhythmGame.perfectCount, rhythmGame.goodCount, rhythmGame.missCount);
}
