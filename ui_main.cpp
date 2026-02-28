/*
 * ui_main.cpp — Main (home) view
 * ────────────────────────────────
 * Pet display, stat bars, action bar with cursor.
 */
#include "ui_main.h"
#include "ui_common.h"
#include "pet.h"
#include "nav.h"

// ── Local helpers ─────────────────────────────────────────
static void drawStatusBar() {
  uint16_t bg = COL_BG_MAIN;
  uint32_t sec = millis() / 1000;
  char tbuf[6];
  sprintf(tbuf, "%02d:%02d", (int)(sec/3600)%24, (int)(sec/60)%60);
  gfx->setTextColor(COL_CYAN);  gfx->setTextSize(1);
  gfx->setCursor(8, 8);         gfx->print(tbuf);

  gfx->setTextColor(COL_PINK);
  gfx->setCursor(178, 8);
  gfx->print(animFrame ? "<3" : " 3");

  gfx->fillCircle(225, 11, 3, animFrame ? COL_GREEN : COL_DIM);

  gfx->drawFastHLine(0, 22, SCREEN_W, COL_DIM);
}

static void drawDecorStars() {
  const int sx[] = {20, 80, 170, 200, 55, 140, 35};
  const int sy[] = {40, 52,  36,  58, 32,  45, 60};
  for (int i = 0; i < 7; i++) {
    uint16_t c = (i < 5) ? COL_WHITE : COL_DIM;
    gfx->drawPixel(sx[i], sy[i], c);
    if (i < 3) {
      gfx->drawPixel(sx[i]+1, sy[i], c);
      gfx->drawPixel(sx[i], sy[i]+1, c);
      gfx->drawPixel(sx[i]+1, sy[i]+1, c);
    }
  }
}

static void drawGroundLine() {
  for (int x = 20; x < 220; x += 2) {
    uint16_t gc = (x < 50 || x > 190) ? COL_DARK : COL_DIM;
    gfx->drawPixel(x, 183, gc);
  }
}

static void drawPetNameMood() {
  gfx->setTextColor(COL_CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(105, 154);
  gfx->print("BYTE");

  gfx->setTextColor(COL_YELLOW);
  const char* ms = petGetMoodString();
  gfx->setCursor((SCREEN_W - (int)strlen(ms)*6)/2, 168);
  gfx->print(ms);
}

// ══════════════════════════════════════════════════════════
//  FULL DRAW
// ══════════════════════════════════════════════════════════

void uiMainDraw() {
  drawStatusBar();
  drawDecorStars();
  drawPixelPet(120, 112, petGetMood(), false);
  drawPetNameMood();
  drawGroundLine();
  uiMainDrawStatBars();
  uiMainDrawActionBar();
}

// ══════════════════════════════════════════════════════════
//  STAT BARS (also called from decay tick)
// ══════════════════════════════════════════════════════════

void uiMainDrawStatBars() {
  uint16_t bg = navViewBgColor();
  int panelY = 188;

  gfx->drawFastHLine(0, panelY - 2, SCREEN_W, COL_DIM);

  const char*    lbls[] = {"HP  ", "FOOD", "JOY ", "NRG "};
  const uint8_t  vals[] = {pet.hp, pet.hunger, pet.happy, pet.energy};
  const uint16_t cols[] = {COL_GREEN, COL_ORANGE, COL_PINK, COL_CYAN};

  for (int i = 0; i < 4; i++) {
    int y = panelY + i * 13;

    gfx->fillRect(0, y, SCREEN_W, 12, bg);

    gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
    gfx->setCursor(6, y + 2);   gfx->print(lbls[i]);

    drawBarWithBorder(36, y + 1, 162, 7, vals[i], cols[i]);

    char b[5]; sprintf(b, "%3d", vals[i]);
    gfx->setTextColor(COL_DIM); gfx->setCursor(204, y + 2); gfx->print(b);
  }
}

// ══════════════════════════════════════════════════════════
//  ACTION BAR (also called on cursor change)
// ══════════════════════════════════════════════════════════

void uiMainDrawActionBar() {
  uint16_t bg = COL_BG_MAIN;
  int barY = 242;

  gfx->fillRect(0, barY - 2, SCREEN_W, SCREEN_H - barY + 2, bg);
  gfx->drawFastHLine(0, barY - 2, SCREEN_W, COL_DIM);

  const char*    icons[] = {"EAT", "PLAY", "ZZZ", "INFO"};
  const uint16_t icols[] = {COL_ORANGE, COL_PINK, COL_CYAN, COL_PURPLE};
  const int      boxW    = 52;
  const int      boxH    = 28;
  const int      gap     = 6;
  int totalW = 4 * boxW + 3 * gap;
  int startX = (SCREEN_W - totalW) / 2;

  for (int i = 0; i < 4; i++) {
    int bx = startX + i * (boxW + gap);
    int by = barY + 2;
    bool sel = (i == actionCursor);

    if (sel) {
      gfx->fillRoundRect(bx, by, boxW, boxH, 4, COL_SEL_BG);
      gfx->drawRoundRect(bx, by, boxW, boxH, 4, icols[i]);
    } else {
      gfx->fillRoundRect(bx, by, boxW, boxH, 4, COL_DARK);
      gfx->drawRoundRect(bx, by, boxW, boxH, 4, COL_DIM);
    }

    int tw = strlen(icons[i]) * 6;
    gfx->setTextColor(sel ? icols[i] : COL_DIM);
    gfx->setTextSize(1);
    gfx->setCursor(bx + (boxW - tw)/2, by + 10);
    gfx->print(icons[i]);
  }

  gfx->setTextColor(COL_DIM);
  gfx->setTextSize(1);
  gfx->setCursor(42, 276);
  gfx->print("A=NEXT   B=SELECT");
}

// ══════════════════════════════════════════════════════════
//  PARTIAL ANIMATION (no fillScreen)
// ══════════════════════════════════════════════════════════

void uiMainAnimate() {
  uint16_t bg = COL_BG_MAIN;

  // Clear pet + mood area
  gfx->fillRect(30, 68, 180, 116, bg);

  if (animFrame) {
    gfx->drawPixel(55, 75, COL_WHITE);
    gfx->drawPixel(170, 82, COL_WHITE);
  }

  int petY = animFrame ? 114 : 110;
  drawPixelPet(120, petY, petGetMood(), false);
  drawPetNameMood();
  drawGroundLine();

  // Update clock area
  gfx->fillRect(8, 4, 40, 14, bg);
  uint32_t sec = millis() / 1000;
  char tbuf[6];
  sprintf(tbuf, "%02d:%02d", (int)(sec/3600)%24, (int)(sec/60)%60);
  gfx->setTextColor(COL_CYAN); gfx->setTextSize(1);
  gfx->setCursor(8, 8); gfx->print(tbuf);

  // Heartbeat
  gfx->fillRect(174, 4, 30, 14, bg);
  gfx->setTextColor(COL_PINK);
  gfx->setCursor(178, 8);
  gfx->print(animFrame ? "<3" : " 3");

  gfx->fillCircle(225, 11, 3, animFrame ? COL_GREEN : COL_DIM);
}
