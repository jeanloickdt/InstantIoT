#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v1.1.0
 * 
 * InstantIoTColor.hpp — RGB color utilities
 * 
 * Usage:
 *   Color c = Color::fromHSV(120, 1.0f, 1.0f); // green
 *   instant.led("led1").setColor(c.r, c.g, c.b);
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>

namespace InstantIoT {

/**
 * RGB color with utilities for IoT use cases
 */
struct Color {
    uint8_t r, g, b;

    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    // ── Presets ───────────────────────────────────────────────
    static Color black()   { return Color(0,   0,   0);   }
    static Color white()   { return Color(255, 255, 255); }
    static Color red()     { return Color(255, 0,   0);   }
    static Color green()   { return Color(0,   255, 0);   }
    static Color blue()    { return Color(0,   0,   255); }
    static Color yellow()  { return Color(255, 255, 0);   }
    static Color cyan()    { return Color(0,   255, 255); }
    static Color magenta() { return Color(255, 0,   255); }
    static Color orange()  { return Color(255, 165, 0);   }

    // ── Conversions ───────────────────────────────────────────

    uint32_t toUint32() const {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    static Color fromUint32(uint32_t color) {
        return Color((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }

    static Color fromHex(const char* hex) {
        if (!hex) return Color();
        if (hex[0] == '#') hex++;
        uint32_t value = 0;
        for (int i = 0; i < 6 && hex[i]; i++) {
            value <<= 4;
            char c = hex[i];
            if      (c >= '0' && c <= '9') value |= (c - '0');
            else if (c >= 'A' && c <= 'F') value |= (c - 'A' + 10);
            else if (c >= 'a' && c <= 'f') value |= (c - 'a' + 10);
        }
        return fromUint32(value);
    }

    /**
     * Create from HSV — useful for color wheels and animations
     * @param h Hue 0..360
     * @param s Saturation 0.0..1.0
     * @param v Value 0.0..1.0
     */
    static Color fromHSV(float h, float s, float v) {
        h = fmod(h, 360.0f);
        if (h < 0) h += 360.0f;
        float c  = v * s;
        float x  = c * (1.0f - fabsf(fmod(h / 60.0f, 2.0f) - 1.0f));
        float m  = v - c;
        float r1, g1, b1;
        if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
        else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
        else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
        else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
        else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
        else              { r1 = c; g1 = 0; b1 = x; }
        return Color((r1 + m) * 255, (g1 + m) * 255, (b1 + m) * 255);
    }

    // ── Effects ───────────────────────────────────────────────

    Color lerp(const Color& other, float t) const {
        t = constrain(t, 0.0f, 1.0f);
        return Color(
            r + (other.r - r) * t,
            g + (other.g - g) * t,
            b + (other.b - b) * t
        );
    }

    Color dim(float factor) const {
        factor = constrain(factor, 0.0f, 1.0f);
        return Color(r * factor, g * factor, b * factor);
    }

    // ── Operators ─────────────────────────────────────────────

    bool operator==(const Color& o) const { return r==o.r && g==o.g && b==o.b; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

} // namespace InstantIoT