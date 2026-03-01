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

static void drawBallAt(float bx, float by, uint16_t color) {
  // Convert 0-100 game coords to screen pixels inside the game area
  int sx = GAME_X + (int)(bx * GAME_W / 100.0f);
  int sy = GAME_Y + (int)(by * GAME_H / 80.0f);
  sx = constrain(sx, GAME_X + 3, GAME_X + GAME_W - 3);
  sy = constrain(sy, GAME_Y + 3, GAME_Y + GAME_H - 3);
  gfx->fillCircle(sx, sy, 3, color);
}

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

// Track previous ball position so we can erase it without full redraw
static float prevBallX = -1, prevBallY = -1;

void uiPlayBalanceAnimate() {
  // Erase previous ball position
  if (prevBallX >= 0) {
    // Redraw the cell under the old ball position
    const uint8_t* maze = balanceGameGetMazePattern();
    int cx = (int)(prevBallX * 10 / 100.0f);  // Game X (0-100) → cell col (0-10)
    int cy = (int)(prevBallY / 10.0f);         // Game Y (0-80, cell_size=10) → cell row (0-8)
    cx = constrain(cx, 0, 9);
    cy = constrain(cy, 0, 7);
    uint8_t cell = maze[cy * 10 + cx];
    uint16_t color = (cell == MAZE_WALL) ? COL_WALL_C :
                     (cell == MAZE_GOAL) ? COL_GOAL_C : COL_CELL_C;
    drawBallAt(prevBallX, prevBallY, color);
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
