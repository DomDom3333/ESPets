/*
 * ui_sleep.cpp — Sleep view
 * ──────────────────────────
 * Sleeping pet with animated Zzz and recovery bars.
 */
#include "ui_sleep.h"
#include "ui_common.h"
#include "nav.h"

// ── Local helper: draw a labelled recovery bar ────────────
static void drawRecoveryBar(int y, const char* lbl,
                            uint8_t val, uint16_t col) {
  gfx->setTextColor(COL_DIM);
  gfx->setCursor(30, y);
  gfx->print(lbl);

  drawBarWithBorder(30, y + 12, 180, 8, val, col);

  char b[6]; sprintf(b, "%d%%", val);
  gfx->setTextColor(col);
  gfx->setCursor(214, y + 12);
  gfx->print(b);
}

// ══════════════════════════════════════════════════════════
//  FULL DRAW
// ══════════════════════════════════════════════════════════

void uiSleepDraw() {
  gfx->setTextColor(COL_CYAN); gfx->setTextSize(1);
  gfx->setCursor(62, 40);       gfx->print("BYTE IS SLEEPING");

  drawPixelPet(120, 128, 's', false);

  // Zzz
  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  gfx->setCursor(150, 76);      gfx->print("z");
  gfx->setTextSize(1);
  gfx->setCursor(170, 62);      gfx->print("z");

  // Recovery bars
  drawRecoveryBar(200, "ENERGY RESTORING:", pet.energy, COL_CYAN);
  drawRecoveryBar(228, "HP RECOVERING:",    pet.hp,     COL_GREEN);

  // Hint
  gfx->setTextColor(COL_DIM); gfx->setCursor(32, 262);
  gfx->print("[ A or B ] WAKE UP");
}

// ══════════════════════════════════════════════════════════
//  PARTIAL ANIMATION (pet bob + bar updates)
// ══════════════════════════════════════════════════════════

void uiSleepAnimate() {
  uint16_t bg = COL_BG_SLEEP;

  // Clear pet + zzz area
  gfx->fillRect(30, 56, 180, 110, bg);

  int petY = animFrame ? 130 : 126;
  drawPixelPet(120, petY, 's', false);

  // Animated Zzz
  gfx->setTextColor(COL_YELLOW); gfx->setTextSize(2);
  gfx->setCursor(150, animFrame ? 78 : 74); gfx->print("z");
  gfx->setTextSize(1);
  gfx->setCursor(170, animFrame ? 64 : 60); gfx->print("z");

  // Update energy bar
  gfx->fillRect(30, 212, 186, 8, bg);
  drawBarWithBorder(30, 212, 180, 8, pet.energy, COL_CYAN);
  char b[6]; sprintf(b, "%d%%", pet.energy);
  gfx->fillRect(214, 212, 26, 8, bg);
  gfx->setTextColor(COL_CYAN); gfx->setCursor(214, 212); gfx->print(b);

  // Update HP bar
  gfx->fillRect(30, 240, 186, 8, bg);
  drawBarWithBorder(30, 240, 180, 8, pet.hp, COL_GREEN);
  sprintf(b, "%d%%", pet.hp);
  gfx->fillRect(214, 240, 26, 8, bg);
  gfx->setTextColor(COL_GREEN); gfx->setCursor(214, 240); gfx->print(b);
}
