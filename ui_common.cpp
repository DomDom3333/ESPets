/*
 * ui_common.cpp — Shared drawing helpers
 * ────────────────────────────────────────
 */
#include "ui_common.h"
#include "nav.h"      // navViewBgColor
#include "pet.h"      // petGetMood

// ══════════════════════════════════════════════════════════
//  NOTIFICATION
// ══════════════════════════════════════════════════════════

void triggerNotif(const char* msg) {
  strncpy(notif.msg, msg, 35);
  notif.msg[35]  = '\0';
  notif.active   = true;
  notif.drawn    = false;
  notif.endTime  = millis() + NOTIF_DURATION;
}

void drawNotification() {
  gfx->fillRoundRect(6, 28, 228, 22, 4, COL_BLACK);
  gfx->drawRoundRect(6, 28, 228, 22, 4, COL_YELLOW);
  gfx->setTextColor(COL_YELLOW);
  gfx->setTextSize(1);
  int tw = strlen(notif.msg) * 6;
  int tx = max(14, (SCREEN_W - tw) / 2);
  gfx->setCursor(tx, 35);
  gfx->print(notif.msg);
}

// ══════════════════════════════════════════════════════════
//  SPLASH
// ══════════════════════════════════════════════════════════

void drawSplash() {
  gfx->fillScreen(COL_BG_MAIN);

  gfx->drawRoundRect(40, 42, 160, 50, 8, COL_CYAN);

  gfx->setTextColor(COL_CYAN);
  gfx->setTextSize(3);
  gfx->setCursor(65, 56);
  gfx->print("BYTE");

  gfx->setTextColor(COL_DIM);
  gfx->setTextSize(1);
  gfx->setCursor(52, 102);
  gfx->print("tamagotchi v6.0");
  gfx->setCursor(30, 118);
  gfx->print("ESP32-C6 + 1.69\" LCD");

  drawPixelPet(120, 185, 'h', true);

  gfx->setTextColor(COL_DIM);
  gfx->setCursor(42, 248);
  gfx->print("A = CYCLE   B = SELECT");
  gfx->setCursor(42, 262);
  gfx->print("Hold A = Toggle Sleep");
}

// ══════════════════════════════════════════════════════════
//  PIXEL PET
// ══════════════════════════════════════════════════════════

void drawPixelPet(int cx, int cy, char mood, bool large) {
  int s  = large ? 3 : 2;
  int ox = cx - 8 * s;
  int oy = cy - 10 * s;

  // Body ellipse
  for (int r = 0; r <= 8; r++) {
    int hw = (int)(sqrtf(64.0f - (float)(r * r)) + 0.5f);
    gfx->fillRect(ox + (8-hw)*s, oy + (10+r)*s, hw*2*s, s, COL_CYAN);
    if (r > 0)
      gfx->fillRect(ox + (8-hw)*s, oy + (10-r)*s, hw*2*s, s, COL_CYAN);
  }

  // Belly highlight
  for (int r = 0; r <= 5; r++) {
    int hw = (int)(sqrtf(25.0f - (float)(r * r)) + 0.5f);
    gfx->fillRect(ox + (8-hw)*s, oy + (11+r)*s, hw*2*s, s, COL_BELLY);
    if (r > 0)
      gfx->fillRect(ox + (8-hw)*s, oy + (11-r)*s, hw*2*s, s, COL_BELLY);
  }

  // Ears
  gfx->fillTriangle(ox+2*s, oy+5*s, ox+5*s,  oy,     ox+7*s,  oy+5*s, COL_CYAN);
  gfx->fillTriangle(ox+9*s, oy+5*s, ox+11*s, oy,     ox+14*s, oy+5*s, COL_CYAN);

  uint16_t bg = navViewBgColor();

  // Eyes
  if (mood == 's') {
    gfx->fillRect(ox+4*s, oy+9*s, 3*s, s, bg);
    gfx->fillRect(ox+9*s, oy+9*s, 3*s, s, bg);
  } else {
    gfx->fillCircle(ox+5*s,  oy+9*s, s+1,       bg);
    gfx->fillCircle(ox+11*s, oy+9*s, s+1,       bg);
    gfx->fillCircle(ox+6*s,  oy+8*s, max(1,s/2), COL_WHITE);
    gfx->fillCircle(ox+12*s, oy+8*s, max(1,s/2), COL_WHITE);
  }

  // Mouth
  if (mood == 'h') {
    for (int i = -3; i <= 3; i++) {
      int yoff = (i*i)/3;
      gfx->fillRect(ox+(8+i)*s, oy+(13+yoff)*s, s, s, bg);
    }
    // Blush cheeks
    gfx->fillCircle(ox+3*s,  oy+12*s, s, COL_PINK);
    gfx->fillCircle(ox+13*s, oy+12*s, s, COL_PINK);
  } else if (mood == 'd') {
    for (int i = -3; i <= 3; i++) {
      int yoff = (i*i)/3;
      gfx->fillRect(ox+(8+i)*s, oy+(14-yoff)*s, s, s, bg);
    }
  } else {
    gfx->fillRect(ox+5*s, oy+13*s, 6*s, s, bg);
  }

  // Feet
  gfx->fillCircle(ox+5*s,  oy+19*s, s+1, COL_CYAN);
  gfx->fillCircle(ox+11*s, oy+19*s, s+1, COL_CYAN);
}

// ══════════════════════════════════════════════════════════
//  SHARED WIDGETS
// ══════════════════════════════════════════════════════════

void drawViewHeader(const char* title, uint16_t titleCol,
                    const char* hint,  uint16_t hintCol) {
  gfx->setTextColor(titleCol);
  gfx->setTextSize(1);
  gfx->setCursor(8, 8);
  gfx->print(title);

  gfx->setTextColor(hintCol);
  // Right-align hint
  int hw = strlen(hint) * 6;
  gfx->setCursor(SCREEN_W - hw - 8, 8);
  gfx->print(hint);

  gfx->drawFastHLine(0, 22, SCREEN_W, COL_DIM);
}

void drawBarWithBorder(int x, int y, int w, int h,
                       uint8_t val, uint16_t fillCol) {
  gfx->fillRect(x, y, w, h, COL_BAR_BG);
  gfx->drawRect(x, y, w, h, COL_DARK);
  int fw = ((int)val * (w - 2)) / 100;
  if (fw > 0)
    gfx->fillRect(x + 1, y + 1, fw, h - 2, fillCol);
}
