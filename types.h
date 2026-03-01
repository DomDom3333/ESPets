/*
 * types.h — Shared types & extern state declarations
 * ────────────────────────────────────────────────────
 * Enums, structs, and extern references to global state.
 * Actual definitions live in globals.cpp.
 *
 * PetState includes a seed field for procedural creature
 * generation (derived from ChipId).  Future: `uint8_t flags`
 * bitfield for BLE/ESP-NOW status.
 */
#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

// ── View enumeration ──────────────────────────────────────
enum View {
  VIEW_MAIN,
  VIEW_FEED,
  VIEW_PLAY,           // Star catch game (classic)
  VIEW_PLAY_RHYTHM,    // Rhythm Tap game (new)
  VIEW_PLAY_BALANCE,   // Tilt Maze game (new)
  VIEW_STATUS,
  VIEW_SLEEP
  // Future: VIEW_BLE_PAIR, VIEW_TRADE, VIEW_EVOLVE ...
};

// ── Pet state ─────────────────────────────────────────────
struct PetState {
  uint8_t hp       = 90;
  uint8_t hunger   = 60;
  uint8_t happy    = 75;
  uint8_t energy   = 80;
  uint8_t age      = 3;
  uint8_t weight   = 12;
  bool    sleeping = false;
  uint32_t seed    = 0;       // creature generation seed (ChipId)
  // Future:
  // uint8_t  stage = 0;       // evolution stage
  // uint8_t  flags = 0;       // BLE paired, etc.
};

// ── Food item definition ──────────────────────────────────
struct FoodItem {
  const char* name;
  uint8_t     pts;
  const char* icon;   // text icon for display e.g. "[B]"
};

// ── Notification state ────────────────────────────────────
struct NotifState {
  bool     active   = false;
  bool     drawn    = false;
  char     msg[36]  = "";
  uint32_t endTime  = 0;
};

// ── Star game state ───────────────────────────────────────
struct StarGameState {
  int      x          = 60;
  int      y          = 100;
  int      score      = 0;
  int      bestScore  = 0;
  uint32_t showUntil  = 0;
  bool     visible    = false;
};

// ── Rhythm Tap game state ──────────────────────────────────
struct RhythmGameState {
  int      round           = 0;
  int      beatIndex       = 0;
  bool     roundComplete   = false;
  uint32_t roundStartTime  = 0;
  uint32_t beatInterval    = 1000;
  int      roundScore      = 0;
  int      totalScore      = 0;
  int      bestScore       = 0;
  int      perfectCount    = 0;
  int      goodCount       = 0;
  int      missCount       = 0;
  int      lastAccuracy    = 0;
  char     feedbackMsg[20] = {};
  uint8_t  feedbackAge     = 0;
  uint16_t feedbackColor   = 0;
  uint32_t lastProcessedClick = 0;
  bool     currentBeatProcessed = false;
};

// ── Balance game state (Tilt Maze) ─────────────────────────
// Forward declaration; full definition in game_balance.h
struct BalanceGameState;  // Defined in game_balance.h

// ══════════════════════════════════════════════════════════
//  EXTERN DECLARATIONS — defined in globals.cpp
// ══════════════════════════════════════════════════════════

// Display
extern Arduino_DataBus *bus;
extern Arduino_GFX     *gfx;

// State
extern PetState       pet;
extern View           currentView;
extern View           previousView;
extern NotifState     notif;
extern StarGameState  starGame;
extern RhythmGameState rhythmGame;
extern BalanceGameState* balanceGame;  // Pointer since struct is forward-declared

// Animation
extern int      animFrame;
extern bool     viewDirty;

// Timing
extern uint32_t lastAnimTick;
extern uint32_t lastDecayTick;

// Input (shared so nav.cpp can read cursor)
extern int      actionCursor;   // main view: 0-3
extern int      selectedFood;   // feed view: 0-6

// Food table
extern const FoodItem foods[6];
