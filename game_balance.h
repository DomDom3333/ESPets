/*
 * game_balance.h — "Tilt Maze" mini-game
 * ──────────────────────────────────────
 * Ball physics with accelerometer input, maze collision, goal detection.
 * Pure game logic — no drawing.
 */
#pragma once

#include "types.h"
#include "mpu6050.h"

// Difficulty levels
enum BalanceDifficulty {
  BALANCE_EASY = 0,
  BALANCE_MEDIUM = 1,
  BALANCE_HARD = 2
};

// Maze cell types
#define MAZE_EMPTY   0
#define MAZE_WALL    1
#define MAZE_GOAL    2

// Game state structure
struct BalanceGameState {
  // Ball position (game world coords, 0-100 scale)
  float ballX, ballY;
  float ballVelX, ballVelY;

  // Game state
  int   difficulty      = BALANCE_EASY;
  int   level           = 1;
  int   score           = 0;
  int   bestScore       = 0;
  bool  levelComplete   = false;
  bool  levelFailed     = false;

  // Timing
  uint32_t levelStartTime = 0;
  uint32_t levelTimeLimit = 30000;  // 30 seconds per level

  // Maze
  uint8_t mazePattern[8][10];  // 8 rows x 10 cols of cells
  uint32_t lastMazeRotate = 0;

  // IMU cache
  IMUData lastIMU;
};

extern BalanceGameState* balanceGame;

// Game lifecycle
void balanceGameInit();                  // Called once at boot
void balanceGameReset();                 // Reset to level 1, easy
void balanceGameStartLevel(int difficulty);
void balanceGameUpdate();                // Called every frame
void balanceGameCheckWinCondition();

// Getters for UI
float balanceGameGetBallX();
float balanceGameGetBallY();
int   balanceGameGetScore();
int   balanceGameGetLevel();
bool  balanceGameIsLevelComplete();
bool  balanceGameIsLevelFailed();
const uint8_t* balanceGameGetMazePattern();  // Returns pointer to mazePattern
