/*
 * game_balance.cpp — Tilt Maze game logic
 * ───────────────────────────────────────
 * Ball physics, maze generation, collision detection.
 */
#include "game_balance.h"
#include "mpu6050.h"

// Global game state (allocate dynamically)
BalanceGameState* balanceGame = nullptr;

// Physics constants (tuned from PELLETINO project)
#define TILT_DEADZONE 0.3f    // Ignore tilts < 0.3g to prevent jitter (hysteresis)
#define TILT_SCALE    0.6f    // Much lower sensitivity - PELLETINO divides by 128, we multiply by 0.6
#define DAMPING       0.92f   // Higher damping for smoother response
#define MAX_VELOCITY  3.0f    // Lower max velocity allows fine control without hitting limits
#define BALL_SIZE     4       // Radius in game units
#define CELL_SIZE     10      // Size of maze cells in game units

// Maze rotation timing (ms)
#define MAZE_ROTATE_INTERVAL 2500

// ══════════════════════════════════════════════════════════
//  MAZE GENERATION
// ══════════════════════════════════════════════════════════

static void generateMaze(int difficulty) {
  // Generate a random maze pattern based on difficulty
  // 8x10 grid of cells
  // 0 = empty, 1 = wall, 2 = goal (bottom-right)

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      balanceGame->mazePattern[y][x] = MAZE_EMPTY;
    }
  }

  // Add some walls based on difficulty
  // Easy: 10-15% walls
  // Medium: 20-25% walls
  // Hard: 30-35% walls
  int wallCount = (difficulty == BALANCE_EASY) ? 8 :
                  (difficulty == BALANCE_MEDIUM) ? 16 : 24;

  randomSeed(millis());
  for (int i = 0; i < wallCount; i++) {
    int rx = random(0, 10);
    int ry = random(1, 7);  // Avoid top and bottom rows
    if (rx > 1 || ry > 1) {  // Don't block start
      balanceGame->mazePattern[ry][rx] = MAZE_WALL;
    }
  }

  // Place goal in bottom-right area
  balanceGame->mazePattern[6][9] = MAZE_GOAL;
  balanceGame->mazePattern[7][9] = MAZE_GOAL;
}

static void rotateMaze(int difficulty) {
  // Randomly shuffle some walls (rotation effect)
  for (int i = 0; i < 5; i++) {
    int x1 = random(0, 10);
    int y1 = random(1, 8);
    int x2 = random(0, 10);
    int y2 = random(1, 8);

    // Swap cells
    uint8_t tmp = balanceGame->mazePattern[y1][x1];
    balanceGame->mazePattern[y1][x1] = balanceGame->mazePattern[y2][x2];
    balanceGame->mazePattern[y2][x2] = tmp;
  }
}

// ══════════════════════════════════════════════════════════
//  COLLISION DETECTION
// ══════════════════════════════════════════════════════════

static bool checkCellCollision(float x, float y) {
  // Check if ball collides with a wall at position (x, y)
  // Coordinates are in game units (0-100)

  int cellX = (int)(x / CELL_SIZE);
  int cellY = (int)(y / CELL_SIZE);

  // Bounds check
  if (cellX < 0 || cellX >= 10 || cellY < 0 || cellY >= 8) {
    return true;  // Out of bounds = collision
  }

  return balanceGame->mazePattern[cellY][cellX] == MAZE_WALL;
}

static bool checkGoalCollision(float x, float y) {
  // Check if ball is in goal area
  int cellX = (int)(x / CELL_SIZE);
  int cellY = (int)(y / CELL_SIZE);

  if (cellX < 0 || cellX >= 10 || cellY < 0 || cellY >= 8) {
    return false;
  }

  return balanceGame->mazePattern[cellY][cellX] == MAZE_GOAL;
}

// ══════════════════════════════════════════════════════════
//  GAME LIFECYCLE
// ══════════════════════════════════════════════════════════

void balanceGameInit() {
  // Called once at boot
  if (balanceGame == nullptr) {
    balanceGame = new BalanceGameState();
  }
  balanceGame->bestScore = 0;
  balanceGameReset();
}

void balanceGameReset() {
  balanceGame->level = 1;
  balanceGame->score = 0;
  balanceGame->difficulty = BALANCE_EASY;
  balanceGameStartLevel(BALANCE_EASY);
}

void balanceGameStartLevel(int difficulty) {
  balanceGame->difficulty = difficulty;
  balanceGame->levelComplete = false;
  balanceGame->levelFailed = false;
  balanceGame->levelStartTime = millis();
  balanceGame->lastMazeRotate = millis();

  // Ball starts in center
  balanceGame->ballX = 50.0f;
  balanceGame->ballY = 50.0f;
  balanceGame->ballVelX = 0.0f;
  balanceGame->ballVelY = 0.0f;

  // Generate maze
  generateMaze(difficulty);

  Serial.printf("[BALANCE] Level %d started (difficulty: %d)\n",
                balanceGame->level, difficulty);
}

// ══════════════════════════════════════════════════════════
//  GAME UPDATE
// ══════════════════════════════════════════════════════════

void balanceGameUpdate() {
  // Called every frame (~10-20ms)

  // First time? Log status
  static bool firstCall = true;
  if (firstCall) {
    firstCall = false;
    if (!imuIsCalibrated()) {
      Serial.println("[BALANCE] ✗ IMU NOT CALIBRATED - ball will not move!");
    } else {
      Serial.println("[BALANCE] ✓ IMU ready, starting game");
    }
  }

  if (!imuIsCalibrated() || balanceGame->levelComplete || balanceGame->levelFailed) {
    return;
  }

  // Read IMU data
  IMUData imuData;
  if (!imuRead(imuData)) {
    return;  // IMU not ready
  }

  // Apply low-pass filter (use 0.9 for more responsiveness in games)
  imuApplyLowPassFilter(imuData, 0.9f);
  balanceGame->lastIMU = imuData;

  // ─── PHYSICS ──────────────────────────────────────────
  // Apply dead zone first (hysteresis prevents jitter at boundaries)
  float accelX = (fabs(imuData.accelX) < TILT_DEADZONE) ? 0.0f : imuData.accelX;
  float accelY = (fabs(imuData.accelY) < TILT_DEADZONE) ? 0.0f : imuData.accelY;

  // DEBUG: Log IMU data every second
  static uint32_t lastDebugTime = 0;
  if (millis() - lastDebugTime > 1000) {
    lastDebugTime = millis();
    Serial.printf("[BALANCE] RawAX=%.2f RawAY=%.2f DZoneAX=%.2f DZoneAY=%.2f VX=%.2f VY=%.2f BallXY=(%.1f,%.1f)\n",
                  imuData.accelX, imuData.accelY, accelX, accelY,
                  balanceGame->ballVelX, balanceGame->ballVelY,
                  balanceGame->ballX, balanceGame->ballY);
  }

  // Map accelerometer to velocity (tuned to PELLETINO sensitivity)
  // ax, ay in m/s² (calibrated with gravity removed from Z)
  // At ±2g range, ±9.8 m/s² represents maximum tilt
  balanceGame->ballVelX = balanceGame->ballVelX * DAMPING +
                          (accelX / 9.8f) * TILT_SCALE;
  balanceGame->ballVelY = balanceGame->ballVelY * DAMPING +
                          (accelY / 9.8f) * TILT_SCALE;

  // Clamp velocity
  if (balanceGame->ballVelX > MAX_VELOCITY) balanceGame->ballVelX = MAX_VELOCITY;
  if (balanceGame->ballVelX < -MAX_VELOCITY) balanceGame->ballVelX = -MAX_VELOCITY;
  if (balanceGame->ballVelY > MAX_VELOCITY) balanceGame->ballVelY = MAX_VELOCITY;
  if (balanceGame->ballVelY < -MAX_VELOCITY) balanceGame->ballVelY = -MAX_VELOCITY;

  // Update position
  float newX = balanceGame->ballX + balanceGame->ballVelX;
  float newY = balanceGame->ballY + balanceGame->ballVelY;

  // ─── COLLISION DETECTION ──────────────────────────────
  // Check if new position would hit a wall
  if (checkCellCollision(newX, newY)) {
    // Bounce: reverse and dampen velocity
    balanceGame->ballVelX *= -0.6f;
    balanceGame->ballVelY *= -0.6f;
    // Don't update position
  } else {
    // Safe to move
    balanceGame->ballX = newX;
    balanceGame->ballY = newY;
  }

  // Boundary clamp (shouldn't reach here, but safeguard)
  if (balanceGame->ballX < 0) {
    balanceGame->ballX = 0;
    balanceGame->ballVelX *= -0.6f;
  }
  if (balanceGame->ballX > 100) {
    balanceGame->ballX = 100;
    balanceGame->ballVelX *= -0.6f;
  }
  if (balanceGame->ballY < 0) {
    balanceGame->ballY = 0;
    balanceGame->ballVelY *= -0.6f;
  }
  if (balanceGame->ballY > 80) {
    balanceGame->ballY = 80;
    balanceGame->ballVelY *= -0.6f;
  }

  // ─── GOAL CHECK ───────────────────────────────────────
  if (checkGoalCollision(balanceGame->ballX, balanceGame->ballY)) {
    balanceGame->levelComplete = true;
    uint32_t timeElapsed = millis() - balanceGame->levelStartTime;
    int timeBonus = max(0, (int)(30000 - timeElapsed) / 1000);
    balanceGame->score += 100 + timeBonus;
    if (balanceGame->score > balanceGame->bestScore) {
      balanceGame->bestScore = balanceGame->score;
    }
    Serial.printf("[BALANCE] Level Complete! Score: %d\n", balanceGame->score);
    return;
  }

  // ─── TIMEOUT CHECK ────────────────────────────────────
  if (millis() - balanceGame->levelStartTime > balanceGame->levelTimeLimit) {
    balanceGame->levelFailed = true;
    Serial.printf("[BALANCE] Level Failed (timeout)\n");
    return;
  }

  // ─── MAZE ROTATION ────────────────────────────────────
  if (millis() - balanceGame->lastMazeRotate > MAZE_ROTATE_INTERVAL) {
    balanceGame->lastMazeRotate = millis();
    rotateMaze(balanceGame->difficulty);
    Serial.println("[BALANCE] Maze rotated");
  }
}

void balanceGameCheckWinCondition() {
  if (balanceGame->levelComplete) {
    // Advance to next level
    balanceGame->level++;
    if (balanceGame->level > 3) {
      balanceGame->level = 1;  // Loop back
    }

    int nextDifficulty = min((int)BALANCE_HARD, balanceGame->difficulty + 1);
    if (balanceGame->level == 1 && balanceGame->difficulty == BALANCE_HARD) {
      nextDifficulty = BALANCE_EASY;  // Reset difficulty on cycle
    }

    balanceGameStartLevel(nextDifficulty);
  } else if (balanceGame->levelFailed) {
    // Restart current level
    balanceGameStartLevel(balanceGame->difficulty);
  }
}

// ══════════════════════════════════════════════════════════
//  GETTERS (for UI)
// ══════════════════════════════════════════════════════════

float balanceGameGetBallX() { return balanceGame->ballX; }
float balanceGameGetBallY() { return balanceGame->ballY; }
int   balanceGameGetScore() { return balanceGame->score; }
int   balanceGameGetLevel() { return balanceGame->level; }
bool  balanceGameIsLevelComplete() { return balanceGame->levelComplete; }
bool  balanceGameIsLevelFailed() { return balanceGame->levelFailed; }
const uint8_t* balanceGameGetMazePattern() {
  return (const uint8_t*)balanceGame->mazePattern;
}
