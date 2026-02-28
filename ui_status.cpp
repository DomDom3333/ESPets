/*
 * ui_status.cpp — Status / info view
 * ────────────────────────────────────
 * Key-value stat cards. Future: BLE ID, trade count, seed.
 */
#include "ui_status.h"
#include "ui_common.h"
#include "pet.h"

void uiStatusDraw() {
  drawViewHeader("STATUS", COL_PURPLE, "A/B = BACK");

  char ageStr[12], wStr[10], hpStr[10], scoreStr[8], uptimeStr[12];
  uint32_t up = millis() / 1000;
  sprintf(ageStr,    "%d DAYS", pet.age);
  sprintf(wStr,      "%d G",    pet.weight);
  sprintf(hpStr,     "%d/100",  pet.hp);
  sprintf(scoreStr,  "%d",      starGame.bestScore);
  sprintf(uptimeStr, "%02d:%02d:%02d",
          (int)(up/3600), (int)((up%3600)/60), (int)(up%60));

  const char*    keys[] = {"NAME","AGE","WEIGHT","HP",
                           "MOOD","STAGE","HI-SCORE","UPTIME"};
  const char*    vals[] = {"BYTE", ageStr, wStr, hpStr,
                           petGetMoodString(), "YOUTH",
                           scoreStr, uptimeStr};
  const uint16_t vc[]   = {COL_CYAN, COL_PURPLE, COL_PURPLE, COL_GREEN,
                           COL_PINK, COL_YELLOW, COL_PINK,   COL_DIM};

  for (int i = 0; i < 8; i++) {
    int y = 28 + i * 30;
    gfx->fillRoundRect(4, y, SCREEN_W - 8, 24, 3, COL_CARD);
    gfx->drawRoundRect(4, y, SCREEN_W - 8, 24, 3, COL_CARD_B);
    gfx->setTextColor(COL_DIM); gfx->setTextSize(1);
    gfx->setCursor(12, y + 8);  gfx->print(keys[i]);
    gfx->setTextColor(vc[i]);
    gfx->setCursor(SCREEN_W - (int)strlen(vals[i]) * 6 - 12, y + 8);
    gfx->print(vals[i]);
  }
}
