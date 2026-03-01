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

// Per-level parameters (index 0 = level 1)
static const int LEVEL_WALL_COUNTS[5] = { 6, 10, 16, 22, 30 };
static const int LEVEL_TIME_LIMITS[5] = { 35000, 30000, 27000, 24000, 20000 };
static const int WALL_DRAW_W[5]       = { 20, 16, 12,  8,  5 };
static const int WALL_DRAW_H[5]       = { 18, 14, 10,  7,  5 };

// ══════════════════════════════════════════════════════════
//  MAZE GENERATION
// ══════════════════════════════════════════════════════════

static void generateMaze(int level) {
  // Generate a random maze pattern based on level (1-5)
  // 8x10 grid of cells: 0 = empty, 1 = wall, 2 = goal (bottom-right)

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      balanceGame->mazePattern[y][x] = MAZE_EMPTY;
    }
  }

  int wallCount = LEVEL_WALL_COUNTS[level - 1];

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
  balanceGameStartLevel(1);
}

void balanceGameStartLevel(int level) {
  level = constrain(level, 1, BALANCE_MAX_LEVEL);
  balanceGame->level        = level;
  balanceGame->difficulty   = (level <= 2) ? BALANCE_EASY :
                              (level <= 4) ? BALANCE_MEDIUM : BALANCE_HARD;
  balanceGame->levelTimeLimit = LEVEL_TIME_LIMITS[level - 1];
  balanceGame->levelComplete  = false;
  balanceGame->levelFailed    = false;
  balanceGame->levelStartTime = millis();

  // Recalibrate IMU at game start (device must be held level and still for ~1 second)
  Serial.println("[BALANCE] Recalibrating IMU sensor...");
  imuCalibrate(200);  // 200 samples ≈ 1 second
  Serial.println("[BALANCE] IMU calibration complete!");

  // Ball starts in center
  balanceGame->ballX    = 50.0f;
  balanceGame->ballY    = 50.0f;
  balanceGame->ballVelX = 0.0f;
  balanceGame->ballVelY = 0.0f;

  // Generate maze
  generateMaze(level);

  Serial.printf("[BALANCE] Level %d started\n", level);
}

// ══════════════════════════════════════════════════════════
//  GAME UPDATE
// ══════════════════════════════════════════════════════════

void balanceGameUpdate() {
  // Called every frame (~10-20ms, but IMU only updated every 80ms)

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

  // DEBUG: Report frame rate status
  static uint32_t lastFpsReport = 0;
  if (millis() - lastFpsReport >= 5000) {
    lastFpsReport = millis();
    Serial.println("[BALANCE] Game capped at 60 FPS (IMU updates every 80ms)");
  }

  if (!imuIsCalibrated() || balanceGame->levelComplete || balanceGame->levelFailed) {
    return;
  }

  // Read IMU data at 50Hz (every 20ms) — sensor doesn't update faster than this
  // and hammering I2C every frame (60fps) just returns stale data with overhead
  static uint32_t lastIMURead = 0;
  static IMUData imuData;
  uint32_t nowMs = millis();
  if (nowMs - lastIMURead >= 20) {
    lastIMURead = nowMs;
    if (!imuRead(imuData)) {
      return;  // IMU not ready
    }
    // Apply low-pass filter (0.9 = very responsive, prioritizes fresh data)
    imuApplyLowPassFilter(imuData, 0.9f);
    balanceGame->lastIMU = imuData;
  }

  // ─── PHYSICS ──────────────────────────────────────────
  // Apply dead zone first (hysteresis prevents jitter at boundaries)
  // NOTE: Axes are swapped and inverted to match PELLETINO mapping
  //   - Pitch (accelX) controls vertical (Y)
  //   - Roll (accelY) controls horizontal (X)
  float accelX = (fabs(imuData.accelY) < TILT_DEADZONE) ? 0.0f : -imuData.accelY;  // Roll → X (inverted)
  float accelY = (fabs(imuData.accelX) < TILT_DEADZONE) ? 0.0f : imuData.accelX;   // Pitch → Y

  // DEBUG: Log IMU data every second
  static uint32_t lastDebugTime = 0;
  static float avgAccelX = 0, avgAccelY = 0;
  static int debugSamples = 0;
  avgAccelX = (avgAccelX * debugSamples + imuData.accelX) / (debugSamples + 1);
  avgAccelY = (avgAccelY * debugSamples + imuData.accelY) / (debugSamples + 1);
  debugSamples++;

  if (millis() - lastDebugTime > 1000) {
    lastDebugTime = millis();
    Serial.printf("[BALANCE] AVG_Raw: AX=%.2f AY=%.2f | Now: AX=%.2f AY=%.2f -> Mapped: X=%.2f Y=%.2f | V: X=%.2f Y=%.2f | Ball: (%.1f,%.1f)\n",
                  avgAccelX, avgAccelY, imuData.accelX, imuData.accelY, accelX, accelY,
                  balanceGame->ballVelX, balanceGame->ballVelY,
                  balanceGame->ballX, balanceGame->ballY);
    debugSamples = 0;
    avgAccelX = 0;
    avgAccelY = 0;
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
    int timeBonus = max(0, (int)(balanceGame->levelTimeLimit - timeElapsed) / 1000);
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

}

void balanceGameCheckWinCondition() {
  if (balanceGame->levelComplete) {
    int nextLevel = balanceGame->level + 1;
    if (nextLevel > BALANCE_MAX_LEVEL) nextLevel = 1;  // Loop back (shouldn't reach here — UI handles last level)
    balanceGameStartLevel(nextLevel);
  } else if (balanceGame->levelFailed) {
    // Restart current level
    balanceGameStartLevel(balanceGame->level);
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

void balanceGameGetWallDrawSize(int& w, int& h) {
  int lvl = constrain(balanceGame->level, 1, BALANCE_MAX_LEVEL);
  w = WALL_DRAW_W[lvl - 1];
  h = WALL_DRAW_H[lvl - 1];
}
