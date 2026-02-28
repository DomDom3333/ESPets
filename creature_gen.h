/*
 * creature_gen.h — Procedural creature generation
 * ─────────────────────────────────────────────────
 * Uses ChipId as a deterministic seed to generate
 * unique creature visuals, name, and stat modifiers.
 * Time slowly drifts colors for a living feel.
 */
#pragma once

#include <Arduino.h>

// ── Creature DNA (all traits derived from seed) ─────────────
struct CreatureDNA {
    // Identity
    uint32_t seed;
    char     name[8];           // Generated name (2-3 syllables)

    // Body shape (pixel units at scale=1)
    uint8_t bodyWidth;          // Half-width of ellipse:  6-10
    uint8_t bodyHeight;         // Half-height of ellipse: 8-12
    uint8_t bellySize;          // Belly circle radius:    3-6

    // Base color hues (0-359)
    uint16_t baseHue;
    uint16_t accentHue;

    // Feature styles
    uint8_t earStyle;           // 0=pointed, 1=round, 2=tall, 3=nubs
    uint8_t eyeStyle;           // 0=round, 1=tall, 2=wide, 3=dot
    uint8_t eyeSpacing;         // Half-distance from center: 2-4
    uint8_t mouthWidth;         // Half-width: 2-4
    uint8_t feetStyle;          // 0=round, 1=small, 2=wide
    uint8_t feetSpacing;        // Half-distance from center: 2-4

    // Markings
    uint8_t spotCount;          // 0-3
    int8_t  spotX[3];           // X offsets from body center
    int8_t  spotY[3];           // Y offsets from body center
    bool    hasTail;
    int8_t  tailDir;            // -1=left, 1=right
    bool    hasTopFin;          // Small fin/spike on head

    // Stat modifiers (-10 to +10)
    int8_t hpMod;
    int8_t hungerMod;
    int8_t happyMod;
    int8_t energyMod;

    // Cached colors (updated by creatureUpdateTime)
    uint16_t bodyColor;         // RGB565 body fill
    uint16_t bellyColor;        // RGB565 belly highlight
    uint16_t accentColor;       // RGB565 cheek/accent
    uint16_t spotColor;         // RGB565 spot/marking

    // Time-varying sparkle
    int8_t  sparkleX;
    int8_t  sparkleY;
    bool    sparkleVisible;
};

extern CreatureDNA creatureDNA;

// Initialize DNA from chip ID seed
void creatureInit(uint32_t chipSeed);

// Update time-varying visual properties (call every animation tick)
void creatureUpdateTime(uint32_t nowMs);
