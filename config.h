/*
 * config.h — Hardware & visual constants
 * ───────────────────────────────────────
 * Pins, screen geometry, RGB565 palette, timing.
 * Change hardware wiring or colour scheme here only.
 */
#pragma once

#include <Arduino.h>

// ── Hardware pins (Waveshare ESP32-C6-LCD-1.69) ──────────
#define PIN_DC     3
#define PIN_CS     5
#define PIN_SCK    1
#define PIN_MOSI   2
#define PIN_RST    4
#define PIN_BL     6

// ── Button (active LOW, internal pull-up) ─────────────────
// All actions via one button using the OneButton library:
//   single click  → cycle / next / catch
//   double click  → select / action / back
//   long press    → toggle sleep
#define BTN_PIN    9   // BOOT button (GPIO 9 on ESP32-C6)

// ── I2C (for QMI8658 IMU) ───────────────────────────────
// SDA=8, SCL=7 (confirmed from hardware schematic)
#define I2C_SDA_PIN 8   // I2C data line (GPIO 8)
#define I2C_SCL_PIN 7   // I2C clock line (GPIO 7)

// ── Sensor power enable ───────────────────────────────────
// GPIO15 (BAT_EN) must be HIGH to power the I2C sensor rail
#define PIN_SENSOR_PWR 15

// ── Screen geometry ───────────────────────────────────────
#define SCREEN_W   240
#define SCREEN_H   280

// ── Timing (ms) ───────────────────────────────────────────
#define ANIM_INTERVAL    600    // pet bob / blink cycle
#define DECAY_INTERVAL   10000  // stat decay tick
#define NOTIF_DURATION   2500   // notification display time
// Button timing is handled by the OneButton library (defaults:
// debounce 50 ms, click 400 ms, long-press 800 ms).

// ── RGB565 colour palette ─────────────────────────────────
// View backgrounds
#define COL_BG_MAIN   ((uint16_t)0x0933)  // #0d1f2d dark blue-green
#define COL_BG_FEED   ((uint16_t)0x0920)  // #0d1a0d dark green
#define COL_BG_PLAY   ((uint16_t)0x1013)  // #1a0d1a dark pink
#define COL_BG_STATUS ((uint16_t)0x0813)  // #0d0d1a dark purple
#define COL_BG_SLEEP  ((uint16_t)0x0111)  // very dark blue

// UI accent colours
#define COL_PANEL     ((uint16_t)0x1126)
#define COL_CYAN      ((uint16_t)0x07FF)
#define COL_GREEN     ((uint16_t)0x3FE3)
#define COL_PINK      ((uint16_t)0xF815)
#define COL_YELLOW    ((uint16_t)0xFF60)
#define COL_ORANGE    ((uint16_t)0xFB40)
#define COL_PURPLE    ((uint16_t)0xBBBF)
#define COL_DIM       ((uint16_t)0x31A6)
#define COL_WHITE     ((uint16_t)0xFFFF)
#define COL_BLACK     ((uint16_t)0x0000)
#define COL_DARK      ((uint16_t)0x1082)

// Bars / cards
#define COL_BAR_BG    ((uint16_t)0x0841)  // dark bar background
#define COL_CARD      ((uint16_t)0x0842)  // stat card fill
#define COL_CARD_B    ((uint16_t)0x18C3)  // stat card border
#define COL_BELLY     ((uint16_t)0x1E7F)  // lighter cyan belly
#define COL_SEL_BG    ((uint16_t)0x0A4F)  // action bar selected bg
#define COL_FEED_SEL  ((uint16_t)0x0320)  // feed card selected bg
#define COL_PLAY_BDR  ((uint16_t)0x3813)  // play area border
#define COL_PLAY_BG   ((uint16_t)0x0811)  // play area inner bg
