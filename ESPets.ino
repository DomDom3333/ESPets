/*
 * ════════════════════════════════════════════════════════════
 *  ESPets — Tamagotchi  v6.0 (modular)
 *  Waveshare ESP32-C6-LCD-1.69  ·  ST7789  ·  240×280
 *  Library: Arduino_GFX_Library by moononournation
 *
 *  Architecture:
 *    config.h        Hardware pins, screen dims, RGB565 palette
 *    types.h         Shared enums, structs, extern state
 *    globals.cpp     Global variable definitions
 *    input.h/.cpp    Button polling & debounce
 *    pet.h/.cpp      Pet logic (decay, feed, mood, sleep)
 *    game_star.h/.cpp  Star-catch mini-game logic
 *    nav.h/.cpp      View switching & button dispatch
 *    ui_common.h/.cpp  Shared draw helpers (pet, notif, splash)
 *    ui_main.h/.cpp    Main (home) screen
 *    ui_feed.h/.cpp    Feed screen
 *    ui_play.h/.cpp    Play screen
 *    ui_status.h/.cpp  Status screen
 *    ui_sleep.h/.cpp   Sleep screen
 *    creature_gen.h/.cpp Procedural creature generation from ChipId
 *
 *  Single-button control (BOOT / GPIO 0) via OneButton library:
 *    single click  → cycle / next / catch
 *    double click  → select / action / back
 *    long press    → toggle sleep
 *    hardware RESET button available separately
 *
 *  Future hooks:
 *    - BLE / ESP-NOW communication module
 *    - Additional mini-games (game_*.h/.cpp)
 *    - Persistent storage (NVS)
 * ════════════════════════════════════════════════════════════
 */

#include "config.h"
#include "types.h"
#include "input.h"
#include "pet.h"
#include "creature_gen.h"
#include "game_star.h"
#include "game_rhythm.h"
#include "game_balance.h"
#include "mpu6050.h"
#include "nav.h"
#include "ui_common.h"
#include "ui_main.h"

// ==========================================================
//  SETUP
// ==========================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== ESPets Tamagotchi v6 ===");

  // ── Backlight ─────────────────────────────────────────
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  // ── Input ─────────────────────────────────────────────
  inputInit();

  // ── Display ───────────────────────────────────────────
  bus = new Arduino_HWSPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
  gfx = new Arduino_ST7789(bus, PIN_RST, 0, false, 240, 280, 0, 20);

  if (!gfx->begin()) {
    // Distress blink if display init fails
    while (true) {
      digitalWrite(PIN_BL, HIGH); delay(150);
      digitalWrite(PIN_BL, LOW);  delay(150);
    }
  }

  gfx->fillScreen(COL_BG_MAIN);

  // ── IMU (MPU6050) ──────────────────────────────────────
  if (imuInit()) {
    // Calibration: device must be stationary for ~1 second
    Serial.println("Keep device stationary for calibration (1 second)...");
    delay(500);
    imuCalibrate(200);  // 200 samples @ 5ms = ~1 second
  } else {
    Serial.println("[WARN] MPU6050 not detected - tilt games disabled");
  }

  // ── Creature generation (from ChipId) ──────────────────
  uint64_t mac = ESP.getEfuseMac();
  uint32_t chipSeed = (uint32_t)(mac ^ (mac >> 32));
  creatureInit(chipSeed);
  petApplyDNA();
  pet.seed = chipSeed;
  creatureUpdateTime(millis());

  // ── Splash ────────────────────────────────────────────
  drawSplash();
  delay(2500);

  // ── Init game & state ─────────────────────────────────
  randomSeed((uint32_t)esp_random());
  starGameReset();
  rhythmGameInit();
  balanceGameInit();
  viewDirty = true;

  Serial.println("=== Boot complete ===");
}

// ==========================================================
//  LOOP
// ==========================================================
void loop() {
  uint32_t now = millis();

  // ── Input ─────────────────────────────────────────────
  inputUpdate();

  // ── IMU polling (every 80ms for smooth filtered data) ──
  static uint32_t lastIMUPoll = 0;
  if (imuIsCalibrated() && now - lastIMUPoll >= 80) {
    lastIMUPoll = now;
    // Raw IMU reading happens here; game logic will read via imuRead()
    IMUData dummy;
    imuRead(dummy);
  }

  // ── Game updates ───────────────────────────────────────
  if (currentView == VIEW_PLAY_RHYTHM) {
    rhythmGameUpdate();
  } else if (currentView == VIEW_PLAY_BALANCE) {
    balanceGameUpdate();
  }

  // ── Full redraw (view switch or forced) ───────────────
  if (viewDirty) {
    viewDirty = false;
    navDrawFullView();
  }

  // ── Partial animation ─────────────────────────────────
  if (now - lastAnimTick >= ANIM_INTERVAL) {
    lastAnimTick = now;
    animFrame ^= 1;
    creatureUpdateTime(now);
    navUpdateAnimation();
  }

  // ── Stat decay ────────────────────────────────────────
  if (now - lastDecayTick >= DECAY_INTERVAL) {
    lastDecayTick = now;
    petTickDecay();
    // Update bars on main view without full redraw
    if (currentView == VIEW_MAIN) uiMainDrawStatBars();
    if (currentView == VIEW_SLEEP) viewDirty = true;
  }

  // ── Notification lifecycle ────────────────────────────
  if (notif.active && now >= notif.endTime) {
    notif.active = false;
    gfx->fillRect(0, 24, SCREEN_W, 28, navViewBgColor());
    notif.drawn = false;
    viewDirty = true;
  }
  if (notif.active && !notif.drawn) {
    drawNotification();
    notif.drawn = true;
  }
}
