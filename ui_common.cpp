/*
 * ui_common.cpp — Shared drawing helpers
 * ────────────────────────────────────────
 */
#include "ui_common.h"
#include "creature_gen.h"
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

  gfx->drawRoundRect(40, 42, 160, 50, 8, creatureDNA.bodyColor);

  gfx->setTextColor(creatureDNA.bodyColor);
  gfx->setTextSize(3);
  int nameLen = strlen(creatureDNA.name);
  int nameW   = nameLen * 18;    // 6px * textSize(3)
  gfx->setCursor((SCREEN_W - nameW) / 2, 56);
  gfx->print(creatureDNA.name);

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
//  PIXEL PET  — procedurally shaped from creatureDNA
// ══════════════════════════════════════════════════════════

void drawPixelPet(int cx, int cy, char mood, bool large) {
  const CreatureDNA& d = creatureDNA;
  int s  = large ? 3 : 2;
  uint16_t bg = navViewBgColor();

  int bw = d.bodyWidth;     // half-width  6-10
  int bh = d.bodyHeight;    // half-height 8-12
  int bs = d.bellySize;     // belly radius 3-6

  // ── Tail (behind body) ────────────────────────────────
  if (d.hasTail) {
    for (int i = 0; i < 3; i++)
      gfx->fillCircle(cx + d.tailDir * (bw + 1 + i) * s,
                       cy + (bh - 3 - i) * s, s, d.bodyColor);
  }

  // ── Body (ellipse) ───────────────────────────────────
  float bhSq = (float)(bh * bh);
  for (int r = 0; r <= bh; r++) {
    int hw = (int)(bw * sqrtf(1.0f - (float)(r * r) / bhSq) + 0.5f);
    gfx->fillRect(cx - hw * s, cy + r * s, hw * 2 * s, s, d.bodyColor);
    if (r > 0)
      gfx->fillRect(cx - hw * s, cy - r * s, hw * 2 * s, s, d.bodyColor);
  }

  // ── Belly highlight ──────────────────────────────────
  float bsSq = (float)(bs * bs);
  for (int r = 0; r <= bs; r++) {
    int hw = (int)(bs * sqrtf(1.0f - (float)(r * r) / bsSq) + 0.5f);
    gfx->fillRect(cx - hw * s, cy + (1 + r) * s, hw * 2 * s, s, d.bellyColor);
    if (r > 0)
      gfx->fillRect(cx - hw * s, cy + (1 - r) * s, hw * 2 * s, s, d.bellyColor);
  }

  // ── Spots ────────────────────────────────────────────
  for (int i = 0; i < d.spotCount; i++)
    gfx->fillCircle(cx + d.spotX[i] * s,
                     cy + d.spotY[i] * s, s, d.spotColor);

  // ── Sparkle (time-varying highlight) ─────────────────
  if (d.sparkleVisible)
    gfx->fillCircle(cx + d.sparkleX * s,
                     cy + d.sparkleY * s, max(1, s / 2), COL_WHITE);

  // ── Ears ─────────────────────────────────────────────
  int earBaseY = cy - (bh - 2) * s;
  int earLX    = cx - (bw * 2 / 3 + 1) * s;
  int earRX    = cx + (bw * 2 / 3 + 1) * s;
  int earHW    = max(1, bw / 3) * s;

  switch (d.earStyle) {
    case 0: {   // Pointed (cat)
      int tipY = earBaseY - 4 * s;
      gfx->fillTriangle(earLX - earHW, earBaseY, earLX, tipY,
                         earLX + earHW, earBaseY, d.bodyColor);
      gfx->fillTriangle(earRX - earHW, earBaseY, earRX, tipY,
                         earRX + earHW, earBaseY, d.bodyColor);
      break;
    }
    case 1: {   // Round (bear)
      int earR = earHW + s;
      gfx->fillCircle(earLX, earBaseY - s, earR, d.bodyColor);
      gfx->fillCircle(earRX, earBaseY - s, earR, d.bodyColor);
      break;
    }
    case 2: {   // Tall (rabbit)
      int tipY    = earBaseY - 7 * s;
      int narrowW = max(1, earHW / 2);
      gfx->fillTriangle(earLX - narrowW, earBaseY, earLX, tipY,
                         earLX + narrowW, earBaseY, d.bodyColor);
      gfx->fillTriangle(earRX - narrowW, earBaseY, earRX, tipY,
                         earRX + narrowW, earBaseY, d.bodyColor);
      break;
    }
    case 3: {   // Nubs
      gfx->fillCircle(earLX, cy - bh * s, earHW, d.bodyColor);
      gfx->fillCircle(earRX, cy - bh * s, earHW, d.bodyColor);
      break;
    }
  }

  // ── Top fin ──────────────────────────────────────────
  if (d.hasTopFin) {
    int finBase = cy - bh * s;
    int finTip  = finBase - 3 * s;
    gfx->fillTriangle(cx - s, finBase, cx, finTip,
                       cx + s, finBase, d.bodyColor);
  }

  // ── Eyes ──────────────────────────────────────────────
  int eyeY  = cy - 1 * s;
  int eyeLX = cx - d.eyeSpacing * s;
  int eyeRX = cx + d.eyeSpacing * s;

  if (mood == 's') {
    // Sleeping: horizontal lines
    gfx->fillRect(eyeLX - s, eyeY, 3 * s, s, bg);
    gfx->fillRect(eyeRX - s, eyeY, 3 * s, s, bg);
  } else {
    switch (d.eyeStyle) {
      case 0:   // Round
        gfx->fillCircle(eyeLX, eyeY, s + 1, bg);
        gfx->fillCircle(eyeRX, eyeY, s + 1, bg);
        break;
      case 1: { // Tall
        int eh = s + s / 2;
        gfx->fillRoundRect(eyeLX - s / 2, eyeY - eh / 2,
                            s + 1, eh, 1, bg);
        gfx->fillRoundRect(eyeRX - s / 2, eyeY - eh / 2,
                            s + 1, eh, 1, bg);
        break;
      }
      case 2: { // Wide
        int ew = s + s / 2;
        gfx->fillRoundRect(eyeLX - ew / 2, eyeY - s / 2,
                            ew, s + 1, 1, bg);
        gfx->fillRoundRect(eyeRX - ew / 2, eyeY - s / 2,
                            ew, s + 1, 1, bg);
        break;
      }
      case 3:   // Dot
        gfx->fillCircle(eyeLX, eyeY, max(1, s / 2), bg);
        gfx->fillCircle(eyeRX, eyeY, max(1, s / 2), bg);
        break;
    }
    // Eye shine (skip for tiny dots)
    if (d.eyeStyle != 3) {
      gfx->fillCircle(eyeLX + s / 2, eyeY - s / 2,
                       max(1, s / 2), COL_WHITE);
      gfx->fillCircle(eyeRX + s / 2, eyeY - s / 2,
                       max(1, s / 2), COL_WHITE);
    }
  }

  // ── Mouth ─────────────────────────────────────────────
  int mouthY = cy + (bh / 2 + 1) * s;
  int mw     = d.mouthWidth;

  if (mood == 'h') {
    for (int i = -mw; i <= mw; i++) {
      int yoff = (i * i) / max(1, mw);
      gfx->fillRect(cx + i * s, mouthY + yoff * s, s, s, bg);
    }
    // Blush cheeks
    int cheekY = cy + (bh / 2) * s;
    gfx->fillCircle(cx - (d.eyeSpacing + 2) * s, cheekY, s, d.accentColor);
    gfx->fillCircle(cx + (d.eyeSpacing + 2) * s, cheekY, s, d.accentColor);
  } else if (mood == 'd') {
    for (int i = -mw; i <= mw; i++) {
      int yoff = (i * i) / max(1, mw);
      gfx->fillRect(cx + i * s, mouthY + s - yoff * s, s, s, bg);
    }
  } else {
    gfx->fillRect(cx - (mw - 1) * s, mouthY,
                   (mw * 2 - 1) * s, s, bg);
  }

  // ── Feet ──────────────────────────────────────────────
  int feetY  = cy + (bh + 1) * s;
  int feetLX = cx - d.feetSpacing * s;
  int feetRX = cx + d.feetSpacing * s;

  switch (d.feetStyle) {
    case 0:   // Round
      gfx->fillCircle(feetLX, feetY, s + 1, d.bodyColor);
      gfx->fillCircle(feetRX, feetY, s + 1, d.bodyColor);
      break;
    case 1:   // Small
      gfx->fillCircle(feetLX, feetY, s, d.bodyColor);
      gfx->fillCircle(feetRX, feetY, s, d.bodyColor);
      break;
    case 2:   // Wide
      gfx->fillRoundRect(feetLX - s, feetY, s * 3, s + 1, 1, d.bodyColor);
      gfx->fillRoundRect(feetRX - s, feetY, s * 3, s + 1, 1, d.bodyColor);
      break;
  }
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
