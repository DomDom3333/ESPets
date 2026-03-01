/*
 * ui_play_balance.cpp — Tilt Maze game UI rendering
 * ──────────────────────────────────────────────────
 * Maze display, ball rendering, score and state visualization.
 */
#include "ui_play_balance.h"
#include "game_balance.h"

// Rendering constants
#define GAME_X      12   // Game area x offset
#define GAME_Y      32   // Game area y offset
#define GAME_W      216  // Game area width (10 cells * 21.6 px)
#define GAME_H      145  // Game area height (8 rows * 18.1 px)
#define CELL_W      21   // Pixels per maze cell
#define CELL_H      18   // Pixels per maze cell

// Colors
#define COL_WALL    ((uint16_t)0xF800)   // Red
#define COL_GOAL    ((uint16_t)0x3FE3)   // Green
#define COL_BALL    ((uint16_t)0xFF60)   // Yellow
#define COL_EMPTY   ((uint16_t)0x0811)   // Dark bg
#define COL_BORDER  ((uint16_t)0x07FF)   // Cyan

// ══════════════════════════════════════════════════════════
//  HELPERS
// ══════════════════════════════════════════════════════════

static void drawMazeCell(int cellX, int cellY, uint8_t cellType) {
  // Convert cell coordinates to screen coordinates
  int sx = GAME_X + (cellX * CELL_W);
  int sy = GAME_Y + (cellY * CELL_H);

  uint16_t color = COL_EMPTY;
  if (cellType == MAZE_WALL) {
    color = COL_WALL;
  } else if (cellType == MAZE_GOAL) {
    color = COL_GOAL;
  }

  gfx->fillRect(sx, sy, CELL_W, CELL_H, color);
}

static void drawBall() {
  // Get ball position in game world (0-100 scale)
  float ballX = balanceGameGetBallX();
  float ballY = balanceGameGetBallY();

  // Convert to screen coordinates
  // Game area is GAME_W x GAME_H pixels, representing 100x80 game units
  int screenX = GAME_X + (int)(ballX * GAME_W / 100.0f);
  int screenY = GAME_Y + (int)(ballY * GAME_H / 80.0f);

  // Draw ball as small circle (2-pixel radius)
  gfx->fillCircle(screenX, screenY, 2, COL_BALL);
  gfx->drawCircle(screenX, screenY, 2, ((uint16_t)0xFFFF));  // White outline
}

// ══════════════════════════════════════════════════════════
//  FULL DRAW (on view entry)
// ══════════════════════════════════════════════════════════

void uiPlayBalanceDraw() {
  // Full screen redraw
  gfx->fillScreen(((uint16_t)0x1013));  // COL_BG_PLAY

  // ─── HEADER ───────────────────────────────────────────────
  gfx->fillRect(0, 0, SCREEN_W, 24, ((uint16_t)0x1082));  // COL_DARK
  gfx->setTextColor(((uint16_t)0x07FF), ((uint16_t)0x1082));  // COL_CYAN
  gfx->setFont(&FreeSans9pt7b);
  gfx->setCursor(8, 18);
  gfx->println("TILT MAZE");

  // Level and timer (top right)
  gfx->setCursor(SCREEN_W - 60, 18);
  gfx->printf("L%d", balanceGameGetLevel());

  // ─── GAME AREA (MAZE) ──────────────────────────────────
  gfx->drawRect(GAME_X - 1, GAME_Y - 1, GAME_W + 2, GAME_H + 2, COL_BORDER);

  // Draw maze
  const uint8_t* maze = balanceGameGetMazePattern();
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      uint8_t cellType = maze[y * 10 + x];
      drawMazeCell(x, y, cellType);
    }
  }

  // Draw ball
  drawBall();

  // ─── SCORE DISPLAY ────────────────────────────────────
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(((uint16_t)0xFF60), ((uint16_t)0x1013));  // Yellow
  gfx->setCursor(8, 190);
  gfx->printf("SCORE: %d", balanceGameGetScore());

  // ─── INSTRUCTIONS ─────────────────────────────────────
  gfx->setFont(&FreeSans7pt7b);
  gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0x1013));
  gfx->setCursor(8, 210);
  gfx->println("Tilt to move, reach green!");

  // ─── BEST SCORE ───────────────────────────────────────
  gfx->setCursor(8, 230);
  gfx->printf("Best: %d", balanceGame->bestScore);
}

// ══════════════════════════════════════════════════════════
//  ANIMATION UPDATE (called frequently during gameplay)
// ══════════════════════════════════════════════════════════

void uiPlayBalanceAnimate() {
  // Called every frame to update ball position and game state

  // ─── CLEAR AND REDRAW GAME AREA ───────────────────────
  // Redraw maze (cells might have rotated)
  const uint8_t* maze = balanceGameGetMazePattern();
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      uint8_t cellType = maze[y * 10 + x];
      drawMazeCell(x, y, cellType);
    }
  }

  // Redraw ball
  drawBall();

  // ─── UPDATE SCORE ─────────────────────────────────────
  gfx->fillRect(8, 185, SCREEN_W - 16, 10, ((uint16_t)0x1013));
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(((uint16_t)0xFF60), ((uint16_t)0x1013));
  gfx->setCursor(8, 195);
  gfx->printf("SCORE: %d", balanceGameGetScore());

  // ─── CHECK GAME STATE ─────────────────────────────────
  if (balanceGameIsLevelComplete()) {
    // Show completion message
    gfx->fillRect(40, 80, SCREEN_W - 80, 30, ((uint16_t)0x3FE3));  // Green bg
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0x3FE3));
    gfx->setCursor(SCREEN_W / 2 - 50, 105);
    gfx->println("LEVEL COMPLETE!");

    // Auto-advance after 2 seconds
    static uint32_t completeTime = 0;
    if (completeTime == 0) {
      completeTime = millis();
    }
    if (millis() - completeTime > 2000) {
      balanceGameCheckWinCondition();
      completeTime = 0;
    }
  } else if (balanceGameIsLevelFailed()) {
    // Show failure message
    gfx->fillRect(40, 80, SCREEN_W - 80, 30, ((uint16_t)0xF800));  // Red bg
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(((uint16_t)0xFFFF), ((uint16_t)0xF800));
    gfx->setCursor(SCREEN_W / 2 - 40, 105);
    gfx->println("TIME'S UP!");

    // Auto-retry after 2 seconds
    static uint32_t failTime = 0;
    if (failTime == 0) {
      failTime = millis();
    }
    if (millis() - failTime > 2000) {
      balanceGameCheckWinCondition();
      failTime = 0;
    }
  }
}
