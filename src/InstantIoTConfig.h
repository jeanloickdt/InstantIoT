#pragma once
/**
 * ============================================================
 * ⚙️ InstantIoTConfig.h - Configuration globale
 * ============================================================
 *
 * Supported platforms:
 *   - ESP32
 *   - ESP8266
 *   - Arduino Uno R4 WiFi
 *
 * ============================================================
 */

// ============================================================
// 🔍 DÉTECTION AUTOMATIQUE DE PLATEFORME
// ============================================================

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    #define INSTANTIOT_PLATFORM_ESP32
#elif defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
    #define INSTANTIOT_PLATFORM_ESP8266
#elif defined(ARDUINO_UNOWIFIR4)
    #define INSTANTIOT_PLATFORM_R4
#else
    #warning "InstantIoT: Plateforme non officielle (ESP32, ESP8266 ou Arduino Uno R4 WiFi recommandé)"
#endif

// ============================================================
// 🐛 DEBUG
// ============================================================

#ifndef INSTANTIOT_DEBUG
    #define INSTANTIOT_DEBUG 0
#endif

// ============================================================
// 📏 TAILLES
// ============================================================

#ifndef INSTANTIOT_MAX_WIDGET_ID_LENGTH
    #define INSTANTIOT_MAX_WIDGET_ID_LENGTH 32
#endif

#ifndef INSTANTIOT_MAX_WIDGETS
    #define INSTANTIOT_MAX_WIDGETS 16
#endif

#ifndef INSTANT_RX_BUFFER_SIZE
    #define INSTANT_RX_BUFFER_SIZE 512
#endif

#ifndef INSTANT_TX_BUFFER_SIZE
    #define INSTANT_TX_BUFFER_SIZE 256
#endif

// ============================================================
// 🎛️ WIDGETS ACTIVÉS
// ============================================================

// ── Display (Device → App) ────────────────────────────────
#ifndef INSTANTIOT_WIDGETS_LED
    #define INSTANTIOT_WIDGETS_LED 1
#endif

#ifndef INSTANTIOT_WIDGETS_GAUGE
    #define INSTANTIOT_WIDGETS_GAUGE 1
#endif

#ifndef INSTANTIOT_WIDGETS_METRIC
    #define INSTANTIOT_WIDGETS_METRIC 1
#endif

#ifndef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
    #define INSTANTIOT_WIDGETS_HORIZONTALLEVEL 1
#endif

#ifndef INSTANTIOT_WIDGETS_VERTICALLEVEL
    #define INSTANTIOT_WIDGETS_VERTICALLEVEL 1
#endif

#ifndef INSTANTIOT_WIDGETS_ADVANCEDCHART
    #define INSTANTIOT_WIDGETS_ADVANCEDCHART 1
#endif

#ifndef INSTANTIOT_WIDGETS_TEXT
    #define INSTANTIOT_WIDGETS_TEXT 1
#endif

// ── Controls (App → Device) ───────────────────────────────
#ifndef INSTANTIOT_WIDGETS_SIMPLEBUTTON
    #define INSTANTIOT_WIDGETS_SIMPLEBUTTON 1
#endif

#ifndef INSTANTIOT_WIDGETS_ADVANCEDBUTTON
    #define INSTANTIOT_WIDGETS_ADVANCEDBUTTON 1
#endif

#ifndef INSTANTIOT_WIDGETS_SWITCH
    #define INSTANTIOT_WIDGETS_SWITCH 1
#endif

#ifndef INSTANTIOT_WIDGETS_JOYSTICK
    #define INSTANTIOT_WIDGETS_JOYSTICK 1
#endif

#ifndef INSTANTIOT_WIDGETS_DIRECTIONPAD
    #define INSTANTIOT_WIDGETS_DIRECTIONPAD 1
#endif

#ifndef INSTANTIOT_WIDGETS_HSLIDER
    #define INSTANTIOT_WIDGETS_HSLIDER 1
#endif

#ifndef INSTANTIOT_WIDGETS_VSLIDER
    #define INSTANTIOT_WIDGETS_VSLIDER 1
#endif

#ifndef INSTANTIOT_WIDGETS_SEGSWITCH
    #define INSTANTIOT_WIDGETS_SEGSWITCH 1
#endif

// ============================================================
// 🖨️ DEBUG MACROS
// ============================================================

#if INSTANTIOT_DEBUG
    #define IIOT_LOG(msg)                   Serial.println(F(msg))
    #define IIOT_LOG_VAL(msg, val)          do { Serial.print(F(msg)); Serial.println(val); } while(0)
    #define IIOT_LOG_2(m1,v1,m2,v2)        do { Serial.print(F(m1)); Serial.print(v1); Serial.print(F(m2)); Serial.println(v2); } while(0)
    #define IIOT_LOG_3(m1,v1,m2,v2,m3,v3)  do { Serial.print(F(m1)); Serial.print(v1); Serial.print(F(m2)); Serial.print(v2); Serial.print(F(m3)); Serial.println(v3); } while(0)
#else
    #define IIOT_LOG(msg)
    #define IIOT_LOG_VAL(msg, val)
    #define IIOT_LOG_2(m1,v1,m2,v2)
    #define IIOT_LOG_3(m1,v1,m2,v2,m3,v3)
#endif