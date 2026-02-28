/*
 * creature_gen.cpp — Procedural creature generation
 * ──────────────────────────────────────────────────
 * Deterministic hash derives all creature properties
 * from a single uint32_t seed (the ESP ChipId).
 * Time-based functions slowly shift hue and sparkle.
 */
#include "creature_gen.h"
#include <math.h>

CreatureDNA creatureDNA;

// ══════════════════════════════════════════════════════════════
//  DETERMINISTIC HASH (Knuth multiplicative + avalanche)
// ══════════════════════════════════════════════════════════════

static uint32_t hashTrait(uint32_t seed, uint32_t trait) {
    uint32_t h = seed ^ (trait * 2654435761u);
    h ^= h >> 16;
    h *= 0x45d9f3bu;
    h ^= h >> 16;
    h *= 0x45d9f3bu;
    h ^= h >> 16;
    return h;
}

// ══════════════════════════════════════════════════════════════
//  HSV TO RGB565
// ══════════════════════════════════════════════════════════════
//  h: 0-359   s: 0-255   v: 0-255

static uint16_t hsvToRgb565(uint16_t h, uint8_t s, uint8_t v) {
    h = h % 360;
    uint8_t  region    = h / 60;
    uint16_t remainder = (uint16_t)(h - region * 60) * 255 / 59;

    uint8_t p = ((uint16_t)v * (255 - s)) >> 8;
    uint8_t q = ((uint16_t)v * (255 - (((uint16_t)s * remainder) >> 8))) >> 8;
    uint8_t t = ((uint16_t)v * (255 - (((uint16_t)s * (255 - remainder)) >> 8))) >> 8;

    uint8_t r, g, b;
    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    return ((uint16_t)(r >> 3) << 11) |
           ((uint16_t)(g >> 2) << 5)  |
           (b >> 3);
}

// ══════════════════════════════════════════════════════════════
//  NAME GENERATION  (syllable pairs → 2-3 syllable name)
// ══════════════════════════════════════════════════════════════

static void generateName(uint32_t seed, char* name) {
    static const char cons[]   = "BDFGKLMNPRSTVZ";
    static const char vowels[] = "AEIOU";

    uint32_t h = hashTrait(seed, 100);
    int syllables = (h % 2) + 2;          // 2 or 3 syllables

    int pos = 0;
    for (int i = 0; i < syllables; i++) {
        uint32_t sc = hashTrait(seed, 101 + i * 2);
        uint32_t sv = hashTrait(seed, 102 + i * 2);
        name[pos++] = cons[sc % 14];
        name[pos++] = vowels[sv % 5];
    }
    name[pos] = '\0';
}

// ══════════════════════════════════════════════════════════════
//  INIT — derive all traits from chip seed
// ══════════════════════════════════════════════════════════════

void creatureInit(uint32_t chipSeed) {
    CreatureDNA& d = creatureDNA;
    d.seed = chipSeed;

    // ── Name ──────────────────────────────────────────────
    generateName(chipSeed, d.name);

    // ── Body shape ────────────────────────────────────────
    d.bodyWidth  = (hashTrait(chipSeed, 0) % 5) + 6;    // 6-10
    d.bodyHeight = (hashTrait(chipSeed, 1) % 5) + 8;    // 8-12
    d.bellySize  = (hashTrait(chipSeed, 2) % 4) + 3;    // 3-6

    // ── Colors ────────────────────────────────────────────
    d.baseHue   = hashTrait(chipSeed, 3) % 360;
    d.accentHue = (d.baseHue + 90 + hashTrait(chipSeed, 4) % 180) % 360;

    // ── Features ──────────────────────────────────────────
    d.earStyle    = hashTrait(chipSeed, 5) % 4;
    d.eyeStyle    = hashTrait(chipSeed, 6) % 4;
    d.eyeSpacing  = (hashTrait(chipSeed, 7) % 3) + 2;   // 2-4
    d.mouthWidth  = (hashTrait(chipSeed, 8) % 3) + 2;   // 2-4
    d.feetStyle   = hashTrait(chipSeed, 9) % 3;
    d.feetSpacing = (hashTrait(chipSeed, 10) % 3) + 2;  // 2-4

    // ── Markings ──────────────────────────────────────────
    uint32_t spotRoll = hashTrait(chipSeed, 11) % 8;
    d.spotCount = (spotRoll < 4) ? 0
               : (spotRoll < 6) ? 1
               : (spotRoll < 7) ? 2 : 3;

    for (int i = 0; i < 3; i++) {
        int maxX = max(1, (int)d.bodyWidth  * 3 / 5);
        int maxY = max(1, (int)d.bodyHeight * 3 / 5);
        d.spotX[i] = (int8_t)((int)(hashTrait(chipSeed, 20 + i)
                     % (maxX * 2 + 1)) - maxX);
        d.spotY[i] = (int8_t)((int)(hashTrait(chipSeed, 23 + i)
                     % (maxY * 2 + 1)) - maxY);
    }

    d.hasTail   = (hashTrait(chipSeed, 12) % 3) == 0;   // 33 %
    d.tailDir   = (hashTrait(chipSeed, 13) % 2) ? 1 : -1;
    d.hasTopFin = (hashTrait(chipSeed, 14) % 4) == 0;   // 25 %

    // ── Stat modifiers ────────────────────────────────────
    d.hpMod     = (int8_t)((int)(hashTrait(chipSeed, 15) % 21) - 10);
    d.hungerMod = (int8_t)((int)(hashTrait(chipSeed, 16) % 21) - 10);
    d.happyMod  = (int8_t)((int)(hashTrait(chipSeed, 17) % 21) - 10);
    d.energyMod = (int8_t)((int)(hashTrait(chipSeed, 18) % 21) - 10);

    // ── Initial color computation ─────────────────────────
    creatureUpdateTime(0);

    Serial.print("Creature: ");
    Serial.print(d.name);
    Serial.print("  seed=0x");
    Serial.print(chipSeed, HEX);
    Serial.print("  body=");
    Serial.print(d.bodyWidth);
    Serial.print("x");
    Serial.print(d.bodyHeight);
    Serial.print("  ears=");
    Serial.print(d.earStyle);
    Serial.print("  eyes=");
    Serial.println(d.eyeStyle);
}

// ══════════════════════════════════════════════════════════════
//  TIME UPDATE — slow visual drift
// ══════════════════════════════════════════════════════════════

void creatureUpdateTime(uint32_t nowMs) {
    CreatureDNA& d = creatureDNA;

    // ── Hue drift: ±8 degrees over a 4-hour cycle ────────
    //    4 h = 14 400 000 ms
    float phase = (float)(nowMs % 14400000u) / 14400000.0f;
    int16_t hueDrift = (int16_t)(sinf(phase * 6.2831853f) * 8.0f);

    uint16_t h  = (d.baseHue   + 360 + hueDrift) % 360;
    uint16_t ah = (d.accentHue + 360 + hueDrift / 2) % 360;

    // Body: saturated, medium-bright
    d.bodyColor   = hsvToRgb565(h,  210, 230);
    // Belly: lighter (low saturation, high value)
    d.bellyColor  = hsvToRgb565(h,  100, 250);
    // Accent: complementary hue
    d.accentColor = hsvToRgb565(ah, 190, 240);
    // Spots: slightly offset darker shade
    d.spotColor   = hsvToRgb565((h + 30) % 360, 230, 150);

    // ── Sparkle: repositions every ~2 minutes ─────────────
    uint32_t sparklePhase = nowMs / 120000;
    uint32_t sh = hashTrait(d.seed, 1000 + sparklePhase);
    int maxSX = max(1, (int)d.bodyWidth  - 2);
    int maxSY = max(1, (int)d.bodyHeight - 2);
    d.sparkleX       = (int8_t)((int)(sh % (maxSX * 2 + 1)) - maxSX);
    d.sparkleY       = (int8_t)((int)((sh >> 8) % (maxSY * 2 + 1)) - maxSY);
    d.sparkleVisible = ((sh >> 16) % 3) == 0;   // visible ⅓ of intervals
}
