/*
 * ui_play_balance.cpp — Tilt Maze game UI rendering
 * ──────────────────────────────────────────────────
 * Maze display, ball rendering, score and state visualization.
 */
#include "ui_play_balance.h"
#include "game_balance.h"
#include "ui_common.h"
#include "nav.h"

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

static void drawMazeCell(int cx, int cy, uint8_t cellType) {
  int px = GAME_X + cx * CELL_W;
  int py = GAME_Y + cy * CELL_H;
  // Always clear full cell area first
  gfx->fillRect(px, py, CELL_W - 1, CELL_H - 1, COL_CELL_C);
  if (cellType == MAZE_WALL) {
    int ww, wh;
    balanceGameGetWallDrawSize(ww, wh);
    int offX = ((CELL_W - 1) - ww) / 2;
    int offY = ((CELL_H - 1) - wh) / 2;
    gfx->fillRect(px + offX, py + offY, ww, wh, COL_WALL_C);
  } else if (cellType == MAZE_GOAL) {
    gfx->fillRect(px, py, CELL_W - 1, CELL_H - 1, COL_GOAL_C);
  }
}

static void drawMazeFull() {
  const uint8_t* maze = balanceGameGetMazePattern();
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 10; x++)
      drawMazeCell(x, y, maze[y * 10 + x]);
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

// Erase the ball by filling the bounding box with background color,
// then redrawing all cells in that region. This ensures grid line pixels
// (the gaps between cells) are fully cleared before cells are redrawn.
static void eraseBallAt(float bx, float by) {
  int sx, sy;
  gameToScreen(bx, by, sx, sy);

  // Ball's pixel bounding box (radius 3px)
  int left = sx - 3;
  int right = sx + 3;
  int top = sy - 3;
  int bottom = sy + 3;

  // Fill the entire bounding box with background color to erase all ball pixels
  gfx->fillRect(left, top, right - left + 1, bottom - top + 1, COL_CELL_C);

  // Now redraw all cells that overlap with the bounding box
  const uint8_t* maze = balanceGameGetMazePattern();
  int cxMin = constrain((left - GAME_X) / CELL_W, 0, 9);
  int cxMax = constrain((right - GAME_X) / CELL_W, 0, 9);
  int cyMin = constrain((top - GAME_Y) / CELL_H, 0, 7);
  int cyMax = constrain((bottom - GAME_Y) / CELL_H, 0, 7);

  for (int cy = cyMin; cy <= cyMax; cy++)
    for (int cx = cxMin; cx <= cxMax; cx++)
      drawMazeCell(cx, cy, maze[cy * 10 + cx]);
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
  uint32_t timeLimit = balanceGame->levelTimeLimit;
  uint32_t timeLeft = timeLimit;
  if (balanceGame->levelStartTime > 0) {
    uint32_t elapsed = millis() - balanceGame->levelStartTime;
    timeLeft = (elapsed < timeLimit) ? timeLimit - elapsed : 0;
  }
  int timerFill = (int)(timeLeft * 200 / timeLimit);
  gfx->fillRect(20, GAME_Y + GAME_H + 20, 200, 5, COL_DARK);
  uint16_t tc = (timeLeft > timeLimit / 2) ? COL_GREEN : (timeLeft > timeLimit / 4 ? COL_YELLOW : COL_PINK);
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
  uint32_t timeLimit = balanceGame->levelTimeLimit;
  uint32_t timeLeft = timeLimit;
  if (balanceGame->levelStartTime > 0) {
    uint32_t elapsed = millis() - balanceGame->levelStartTime;
    timeLeft = (elapsed < timeLimit) ? timeLimit - elapsed : 0;
  }
  int timerFill = (int)(timeLeft * 200 / timeLimit);
  gfx->fillRect(20, GAME_Y + GAME_H + 20, 200, 5, COL_DARK);
  uint16_t tc = (timeLeft > timeLimit / 2) ? COL_GREEN : (timeLeft > timeLimit / 4 ? COL_YELLOW : COL_PINK);
  if (timerFill > 0) gfx->fillRect(20, GAME_Y + GAME_H + 20, timerFill, 5, tc);

  // ─── LEVEL COMPLETE OVERLAY ────────────────────────────
  if (balanceGameIsLevelComplete()) {
    bool isLastLevel = (balanceGameGetLevel() == BALANCE_MAX_LEVEL);

    if (isLastLevel) {
      gfx->fillRect(20, 75, SCREEN_W - 40, 56, COL_GREEN);
      gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
      gfx->setCursor(35, 88);
      gfx->print("COMPLETE!");
      gfx->setTextSize(1);
      gfx->setCursor(32, 112);
      gfx->print("JOY +20  Going home...");
    } else {
      gfx->fillRect(30, 80, SCREEN_W - 60, 36, COL_GREEN);
      gfx->setTextColor(COL_WHITE); gfx->setTextSize(2);
      gfx->setCursor(38, 100);
      gfx->print("LEVEL DONE!");
    }

    static uint32_t completeTime = 0;
    if (completeTime == 0) completeTime = millis();
    if (millis() - completeTime > 2000) {
      completeTime = 0;
      prevBallX = -1;
      if (isLastLevel) {
        pet.happy  = (uint8_t)min(100, (int)pet.happy  + 20);
        pet.energy = (uint8_t)max(0,   (int)pet.energy -  5);
        navSwitchView(VIEW_MAIN);
      } else {
        balanceGameCheckWinCondition();
        viewDirty = true;
      }
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
