#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * InstantIoTColor.hpp - Utilitaires de manipulation des couleurs
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>

namespace InstantIoT {

/**
 * @brief Structure représentant une couleur RGB
 */
struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    // Couleurs prédéfinies
    static Color black()   { return Color(0, 0, 0); }
    static Color white()   { return Color(255, 255, 255); }
    static Color red()     { return Color(255, 0, 0); }
    static Color green()   { return Color(0, 255, 0); }
    static Color blue()    { return Color(0, 0, 255); }
    static Color yellow()  { return Color(255, 255, 0); }
    static Color cyan()    { return Color(0, 255, 255); }
    static Color magenta() { return Color(255, 0, 255); }
    static Color orange()  { return Color(255, 165, 0); }
    
    /**
     * @brief Convertit en valeur 32 bits (0x00RRGGBB)
     */
    uint32_t toUint32() const {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
    
    /**
     * @brief Crée depuis une valeur 32 bits
     */
    static Color fromUint32(uint32_t color) {
        return Color(
            (color >> 16) & 0xFF,
            (color >> 8) & 0xFF,
            color & 0xFF
        );
    }
    
    /**
     * @brief Convertit en string hex (#RRGGBB)
     */
    void toHex(char* buffer, size_t bufferSize) const {
        if (bufferSize >= 8) {
            snprintf(buffer, bufferSize, "#%02X%02X%02X", r, g, b);
        }
    }
    
    /**
     * @brief Crée depuis une string hex (#RRGGBB ou RRGGBB)
     */
    static Color fromHex(const char* hex) {
        if (!hex) return Color();
        
        // Skip # si présent
        if (hex[0] == '#') hex++;
        
        // Parse
        uint32_t value = 0;
        for (int i = 0; i < 6 && hex[i]; i++) {
            value <<= 4;
            char c = hex[i];
            if (c >= '0' && c <= '9') value |= (c - '0');
            else if (c >= 'A' && c <= 'F') value |= (c - 'A' + 10);
            else if (c >= 'a' && c <= 'f') value |= (c - 'a' + 10);
        }
        
        return fromUint32(value);
    }
    
    /**
     * @brief Interpole entre deux couleurs
     * @param other Autre couleur
     * @param t Facteur d'interpolation (0.0 - 1.0)
     */
    Color lerp(const Color& other, float t) const {
        t = constrain(t, 0.0f, 1.0f);
        return Color(
            r + (other.r - r) * t,
            g + (other.g - g) * t,
            b + (other.b - b) * t
        );
    }
    
    /**
     * @brief Ajuste la luminosité
     * @param factor Facteur (0.0 = noir, 1.0 = original, >1.0 = plus clair)
     */
    Color brightness(float factor) const {
        return Color(
            constrain(r * factor, 0, 255),
            constrain(g * factor, 0, 255),
            constrain(b * factor, 0, 255)
        );
    }
    
    /**
     * @brief Crée une couleur depuis HSV
     * @param h Hue (0-360)
     * @param s Saturation (0.0-1.0)
     * @param v Value (0.0-1.0)
     */
    static Color fromHSV(float h, float s, float v) {
        h = fmod(h, 360.0f);
        if (h < 0) h += 360.0f;
        
        float c = v * s;
        float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
        float m = v - c;
        
        float r1, g1, b1;
        if (h < 60)       { r1 = c; g1 = x; b1 = 0; }
        else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
        else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
        else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
        else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
        else              { r1 = c; g1 = 0; b1 = x; }
        
        return Color(
            (r1 + m) * 255,
            (g1 + m) * 255,
            (b1 + m) * 255
        );
    }
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Helper pour créer des payloads JSON de couleur
 */
class ColorPayload {
public:
    static void format(char* buffer, size_t bufferSize, const char* key, const Color& color) {
        char hex[8];
        color.toHex(hex, sizeof(hex));
        snprintf(buffer, bufferSize, "{\"%s\":\"%s\"}", key, hex);
    }
    
    static void format(char* buffer, size_t bufferSize, const char* key, const char* hexColor) {
        snprintf(buffer, bufferSize, "{\"%s\":\"%s\"}", key, hexColor);
    }
};

} // namespace InstantIoT
