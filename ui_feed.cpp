/*
 * ui_feed.cpp — Feed view
 * ─────────────────────────
 * 3×2 food grid + BACK row + hunger bar.
 */
#include "ui_feed.h"
#include "ui_common.h"

void uiFeedDraw() {
  drawViewHeader("FEED", COL_GREEN, "A=SCROLL B=SEL");

  // Food grid: 3 columns × 2 rows
  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int x = 6 + col * 78;
    int y = 28 + row * 68;
    bool sel = (i == selectedFood);

    if (sel) {
      gfx->fillRoundRect(x, y, 72, 62, 4, COL_FEED_SEL);
      gfx->drawRoundRect(x, y, 72, 62, 4, COL_GREEN);
    } else {
      gfx->fillRoundRect(x, y, 72, 62, 4, COL_BAR_BG);
      gfx->drawRoundRect(x, y, 72, 62, 4, COL_DIM);
    }

    // Icon
    gfx->setTextColor(sel ? COL_WHITE : COL_DIM);
    gfx->setTextSize(2);
    int iw = strlen(foods[i].icon) * 12;
    gfx->setCursor(x + (72 - iw)/2, y + 8);
    gfx->print(foods[i].icon);

    // Name
    gfx->setTextColor(sel ? COL_WHITE : COL_DIM);
    gfx->setTextSize(1);
    int nw = strlen(foods[i].name) * 6;
    gfx->setCursor(x + (72 - nw)/2, y + 32);
    gfx->print(foods[i].name);

    // Points
    char buf[10]; sprintf(buf, "+%d", foods[i].pts);
    gfx->setTextColor(COL_GREEN);
    int pw = strlen(buf) * 6;
    gfx->setCursor(x + (72 - pw)/2, y + 46);
    gfx->print(buf);
  }

  // BACK option (7th item)
  {
    int x = 6, y = 28 + 2 * 68;
    bool sel = (selectedFood == 6);
    gfx->fillRoundRect(x, y, 228, 24, 4, sel ? COL_DARK : COL_BAR_BG);
    gfx->drawRoundRect(x, y, 228, 24, 4, sel ? COL_CYAN : COL_DIM);
    gfx->setTextColor(sel ? COL_CYAN : COL_DIM);
    gfx->setTextSize(1);
    gfx->setCursor(x + 84, y + 8);
    gfx->print("< BACK >");
  }

  // Hunger bar
  int barY = 224;
  gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
  gfx->setCursor(8, barY);    gfx->print("HUNGER:");
  drawBarWithBorder(60, barY, 140, 7, pet.hunger, COL_ORANGE);
  char hbuf[6]; sprintf(hbuf, "%d%%", pet.hunger);
  gfx->setTextColor(COL_ORANGE); gfx->setCursor(206, barY); gfx->print(hbuf);

  // Prompt
  gfx->setTextColor(COL_DIM);
  gfx->setCursor(24, barY + 16);
  gfx->print(selectedFood < 6 ? "Press B to feed BYTE!" : "Press B to go back");
}
