/*
 * ui_play_balance.cpp — Tilt Maze game UI rendering
 * ──────────────────────────────────────────────────
 * Maze display, ball rendering, score and state visualization.
 */
#include "ui_play_balance.h"
#include "game_balance.h"
#include "ui_common.h"

// Maze rendering geometry (fits within 240x280 screen)
#define GAME_X      8    // Game area x offset
#define GAME_Y      32   // Game area y offset (below header)
#define GAME_W      224  // 10 cells * CELL_W
#define GAME_H      160  // 8 rows * CELL_H
#define CELL_W      22   // Width of each maze cell in pixels
#define CELL_H      20   // Height of each maze cell in pixels

// Colors for maze elements
#define COL_WALL_C  COL_PINK
#define COL_GOAL_C  COL_GREEN
#define COL_BALL_C  COL_YELLOW
#define COL_CELL_C  COL_PLAY_BG

// ══════════════════════════════════════════════════════════
//  HELPERS
// ══════════════════════════════════════════════════════════

static void drawMazeFull() {
  // Draw entire maze grid from current state
  const uint8_t* maze = balanceGameGetMazePattern();
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      uint8_t cell = maze[y * 10 + x];
      uint16_t color = (cell == MAZE_WALL) ? COL_WALL_C :
                       (cell == MAZE_GOAL) ? COL_GOAL_C : COL_CELL_C;
      gfx->fillRect(GAME_X + x * CELL_W, GAME_Y + y * CELL_H,
                    CELL_W - 1, CELL_H - 1, color);
    }
  }
  // Border around maze
  gfx->drawRect(GAME_X - 1, GAME_Y - 1, GAME_W + 2, GAME_H + 2, COL_DIM);
}

static void gameToScreen(float bx, float by, int& sx, int& sy) {
  sx = GAME_X + (int)(bx * GAME_W / 100.0f);
  sy = GAME_Y + (int)(by * GAME_H / 80.0f);
  sx = constrain(sx, GAME_X + 3, GAME_X + GAME_W - 3);
  sy = constrain(sy, GAME_Y + 3, GAME_Y + GAME_H - 3);
}

static void drawBallAt(float bx, float by, uint16_t color) {
  int sx, sy;
  gameToScreen(bx, by, sx, sy);
  gfx->fillCircle(sx, sy, 3, color);
}

// Erase the ball by redrawing every maze cell the ball circle overlaps.
// A single-cell erase is wrong when the 3px radius crosses a cell border.
static void eraseBallAt(float bx, float by) {
  int sx, sy;
  gameToScreen(bx, by, sx, sy);

  // Cell range covered by the ball's bounding box (radius 3px)
  const uint8_t* maze = balanceGameGetMazePattern();
  int cxMin = constrain((sx - 3 - GAME_X) / CELL_W, 0, 9);
  int cxMax = constrain((sx + 3 - GAME_X) / CELL_W, 0, 9);
  int cyMin = constrain((sy - 3 - GAME_Y) / CELL_H, 0, 7);
  int cyMax = constrain((sy + 3 - GAME_Y) / CELL_H, 0, 7);

  // Redraw cells
  for (int cy = cyMin; cy <= cyMax; cy++) {
    for (int cx = cxMin; cx <= cxMax; cx++) {
      uint8_t cell = maze[cy * 10 + cx];
      uint16_t color = (cell == MAZE_WALL) ? COL_WALL_C :
                       (cell == MAZE_GOAL) ? COL_GOAL_C : COL_CELL_C;
      gfx->fillRect(GAME_X + cx * CELL_W, GAME_Y + cy * CELL_H,
                    CELL_W - 1, CELL_H - 1, color);
    }
  }

  // Redraw grid lines in the affected region. Grid lines are 1-pixel gaps
  // between cells (created by drawing cells with CELL_W-1, CELL_H-1 width).
  // The ball can touch these gaps, leaving blue pixels, so redraw them.
  // Vertical grid lines
  for (int cx = cxMin; cx <= cxMax + 1; cx++) {
    if (cx > 0 && cx <= 9) {
      int x = GAME_X + cx * CELL_W - 1;
      int y = GAME_Y + cyMin * CELL_H;
      int h = (cyMax + 1 - cyMin) * CELL_H;
      gfx->drawFastVLine(x, y, h, COL_DIM);
    }
  }
  // Horizontal grid lines
  for (int cy = cyMin; cy <= cyMax + 1; cy++) {
    if (cy > 0 && cy <= 7) {
      int y = GAME_Y + cy * CELL_H - 1;
      int x = GAME_X + cxMin * CELL_W;
      int w = (cxMax + 1 - cxMin) * CELL_W;
      gfx->drawFastHLine(x, y, w, COL_DIM);
    }
  }
}

// ══════════════════════════════════════════════════════════
//  ANIMATION STATE
// ══════════════════════════════════════════════════════════

// Track previous ball position so we can erase it without full redraw
static float prevBallX = -1, prevBallY = -1;

// ══════════════════════════════════════════════════════════
//  FULL DRAW (on view entry)
// ══════════════════════════════════════════════════════════

void uiPlayBalanceDraw() {
  drawViewHeader("TILT MAZE", COL_CYAN, "TILT=MOVE  B=BACK");

  // ─── MAZE ─────────────────────────────────────────────
  gfx->fillRect(GAME_X, GAME_Y, GAME_W, GAME_H, COL_CELL_C);
  drawMazeFull();

  // ─── BALL ─────────────────────────────────────────────
  float ballX = balanceGameGetBallX();
  float ballY = balanceGameGetBallY();
  drawBallAt(ballX, ballY, COL_BALL_C);
  prevBallX = ballX;
  prevBallY = ballY;
  Serial.printf("[BALANCE_UI] Full redraw: ball at (%.1f, %.1f)\n", ballX, ballY);

  // ─── INFO BAR ─────────────────────────────────────────
  gfx->drawFastHLine(0, GAME_Y + GAME_H + 4, SCREEN_W, COL_DIM);

  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, GAME_Y + GAME_H + 14);
  gfx->printf("Lv%d  Score:", balanceGameGetLevel());

  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(1);
  char sbuf[8]; sprintf(sbuf, "%d", balanceGameGetScore());
  gfx->setCursor(112, GAME_Y + GAME_H + 14);
  gfx->print(sbuf);

  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(SCREEN_W - 70, GAME_Y + GAME_H + 14);
  gfx->printf("Best:%d", balanceGame->bestScore);

  // Timer bar
  uint32_t timeLeft = 30000;
  if (balanceGame->levelStartTime > 0) {
    uint32_t elapsed = millis() - balanceGame->levelStartTime;
    timeLeft = (elapsed < 30000) ? 30000 - elapsed : 0;
  }
  int timerFill = (int)(timeLeft * 200 / 30000);
  gfx->fillRect(20, GAME_Y + GAME_H + 20, 200, 5, COL_DARK);
  uint16_t tc = (timeLeft > 15000) ? COL_GREEN : (timeLeft > 7500 ? COL_YELLOW : COL_PINK);
  if (timerFill > 0) gfx->fillRect(20, GAME_Y + GAME_H + 20, timerFill, 5, tc);
  gfx->drawRect(20, GAME_Y + GAME_H + 20, 200, 5, COL_DIM);

  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, GAME_Y + GAME_H + 34);
  gfx->print("Reach the GREEN zone!");
}

// ══════════════════════════════════════════════════════════
//  ANIMATION UPDATE (called frequently during gameplay)
// ══════════════════════════════════════════════════════════

void uiPlayBalanceAnimate() {
  // Erase previous ball position — redraw all cells the circle overlapped
  if (prevBallX >= 0) {
    eraseBallAt(prevBallX, prevBallY);
  }

  float bx = balanceGameGetBallX();
  float by = balanceGameGetBallY();

  // Draw new ball position
  drawBallAt(bx, by, COL_BALL_C);
  prevBallX = bx;
  prevBallY = by;

  // ─── TIMER BAR UPDATE ─────────────────────────────────
  uint32_t timeLeft = 30000;
  if (balanceGame->levelStartTime > 0) {
    uint32_t elapsed = millis() - balanceGame->levelStartTime;
    timeLeft = (elapsed < 30000) ? 30000 - elapsed : 0;
  }
  int timerFill = (int)(timeLeft * 200 / 30000);
  gfx->fillRect(20, GAME_Y + GAME_H + 20, 200, 5, COL_DARK);
  uint16_t tc = (timeLeft > 15000) ? COL_GREEN : (timeLeft > 7500 ? COL_YELLOW : COL_PINK);
  if (timerFill > 0) gfx->fillRect(20, GAME_Y + GAME_H + 20, timerFill, 5, tc);

  // ─── LEVEL COMPLETE OVERLAY ────────────────────────────
  if (balanceGameIsLevelComplete()) {
    gfx->fillRect(30, 80, SCREEN_W - 60, 36, COL_GREEN);
    gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
    gfx->setCursor(38, 100);
    gfx->print("LEVEL DONE!");

    static uint32_t completeTime = 0;
    if (completeTime == 0) completeTime = millis();
    if (millis() - completeTime > 2000) {
      balanceGameCheckWinCondition();
      completeTime = 0;
      prevBallX = -1;  // Force full repaint
      viewDirty = true;
    }
  } else if (balanceGameIsLevelFailed()) {
    gfx->fillRect(30, 80, SCREEN_W - 60, 36, COL_PINK);
    gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
    gfx->setCursor(50, 100);
    gfx->print("TIME'S UP!");

    static uint32_t failTime = 0;
    if (failTime == 0) failTime = millis();
    if (millis() - failTime > 2000) {
      balanceGameCheckWinCondition();
      failTime = 0;
      prevBallX = -1;
      viewDirty = true;
    }
  }
}
