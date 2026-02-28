/*
 * nav.cpp — Navigation & dispatch implementation
 * ────────────────────────────────────────────────
 * Central coordinator: routes button events per view,
 * manages view transitions, dispatches draw calls.
 */
#include "nav.h"
#include "pet.h"
#include "game_star.h"
#include "ui_main.h"
#include "ui_feed.h"
#include "ui_play.h"
#include "ui_status.h"
#include "ui_sleep.h"
#include "ui_common.h"

// ══════════════════════════════════════════════════════════
//  VIEW MANAGEMENT
// ══════════════════════════════════════════════════════════

uint16_t navViewBgColor() {
  switch (currentView) {
    case VIEW_MAIN:   return COL_BG_MAIN;
    case VIEW_FEED:   return COL_BG_FEED;
    case VIEW_PLAY:   return COL_BG_PLAY;
    case VIEW_STATUS: return COL_BG_STATUS;
    case VIEW_SLEEP:  return COL_BG_SLEEP;
    default:          return COL_BG_MAIN;
  }
}

void navSwitchView(View v) {
  previousView = currentView;
  currentView  = v;
  if (v == VIEW_PLAY) { starGame.score = 0; starGameReset(); }
  selectedFood  = 0;
  notif.active  = false;
  notif.drawn   = false;
  viewDirty     = true;
}

// ══════════════════════════════════════════════════════════
//  DRAW DISPATCH
// ══════════════════════════════════════════════════════════

void navDrawFullView() {
  gfx->fillScreen(navViewBgColor());
  switch (currentView) {
    case VIEW_MAIN:   uiMainDraw();   break;
    case VIEW_FEED:   uiFeedDraw();   break;
    case VIEW_PLAY:   uiPlayDraw();   break;
    case VIEW_STATUS: uiStatusDraw(); break;
    case VIEW_SLEEP:  uiSleepDraw();  break;
  }
}

void navUpdateAnimation() {
  switch (currentView) {
    case VIEW_MAIN:  uiMainAnimate();  break;
    case VIEW_SLEEP: uiSleepAnimate(); break;
    case VIEW_PLAY:  uiPlayAnimate();  break;
    default: break;
  }
}

// ══════════════════════════════════════════════════════════
//  BUTTON ACTION DISPATCH
// ══════════════════════════════════════════════════════════

void navOnShortPressA() {
  switch (currentView) {
    case VIEW_MAIN:
      actionCursor = (actionCursor + 1) % 4;
      uiMainDrawActionBar();   // partial redraw
      break;

    case VIEW_FEED:
      selectedFood = (selectedFood + 1) % 7;  // 6 foods + BACK
      viewDirty = true;
      break;

    case VIEW_PLAY:
      starGameCatch();
      viewDirty = true;
      break;

    case VIEW_STATUS:
      navSwitchView(VIEW_MAIN);
      break;

    case VIEW_SLEEP:
      petSetSleeping(false);
      navSwitchView(VIEW_MAIN);
      break;
  }
}

void navOnShortPressB() {
  switch (currentView) {
    case VIEW_MAIN:
      switch (actionCursor) {
        case 0: navSwitchView(VIEW_FEED);   break;
        case 1: navSwitchView(VIEW_PLAY);   break;
        case 2:
          petSetSleeping(true);
          navSwitchView(VIEW_SLEEP);
          break;
        case 3: navSwitchView(VIEW_STATUS); break;
      }
      break;

    case VIEW_FEED:
      if (selectedFood == 6) {
        navSwitchView(VIEW_MAIN);   // BACK
      } else {
        petFeed(selectedFood);
        delay(80);
        navSwitchView(VIEW_MAIN);
      }
      break;

    case VIEW_PLAY:
      navSwitchView(VIEW_MAIN);
      break;

    case VIEW_STATUS:
      navSwitchView(VIEW_MAIN);
      break;

    case VIEW_SLEEP:
      petSetSleeping(false);
      navSwitchView(VIEW_MAIN);
      break;
  }
}

void navOnLongPressA() {
  if (pet.sleeping) {
    petSetSleeping(false);
    navSwitchView(VIEW_MAIN);
  } else {
    petSetSleeping(true);
    navSwitchView(VIEW_SLEEP);
  }
}
