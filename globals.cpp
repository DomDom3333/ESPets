/*
 * globals.cpp — Global variable definitions
 * ───────────────────────────────────────────
 * Single source of truth for all shared state.
 * Everything declared extern in types.h is defined here.
 */
#include "types.h"

// ── Display (assigned in setup, not globally constructed) ─
Arduino_DataBus *bus = nullptr;
Arduino_GFX     *gfx = nullptr;

// ── Core state ────────────────────────────────────────────
PetState       pet;
View           currentView  = VIEW_MAIN;
View           previousView = VIEW_MAIN;
NotifState     notif;
StarGameState  starGame;

// ── Animation ─────────────────────────────────────────────
int      animFrame     = 0;
bool     viewDirty     = true;

// ── Timing ────────────────────────────────────────────────
uint32_t lastAnimTick  = 0;
uint32_t lastDecayTick = 0;

// ── Input cursors ─────────────────────────────────────────
int      actionCursor  = 0;
int      selectedFood  = 0;

// ── Food table ────────────────────────────────────────────
const FoodItem foods[6] = {
  {"BURGER", 15, "[B]"},
  {"APPLE",  10, "[A]"},
  {"PIZZA",  20, "[P]"},
  {"DONUT",   8, "[D]"},
  {"VEGGIE",  5, "[V]"},
  {"RAMEN",  25, "[R]"}
};
