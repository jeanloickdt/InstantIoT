#pragma once
/**
 * ============================================================
 * ⚙️ InstantIoTConfig.h - Global configuration
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
// 🔍 AUTOMATIC PLATFORM DETECTION
// ============================================================

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    #define INSTANTIOT_PLATFORM_ESP32
#elif defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
    #define INSTANTIOT_PLATFORM_ESP8266
#elif defined(ARDUINO_UNOWIFIR4)
    #define INSTANTIOT_PLATFORM_R4
#else
    #warning "InstantIoT: Unofficial platform (ESP32, ESP8266 or Arduino Uno R4 WiFi recommended)"
#endif

// ============================================================
// 🐛 DEBUG
// ============================================================

#ifndef INSTANTIOT_DEBUG
    #define INSTANTIOT_DEBUG 0
#endif

// ============================================================
// 📏 SIZES
// ============================================================

#ifndef INSTANTIOT_MAX_WIDGET_ID_LENGTH
    #define INSTANTIOT_MAX_WIDGET_ID_LENGTH 32
#endif

#ifndef INSTANTIOT_MAX_WIDGETS
    #define INSTANTIOT_MAX_WIDGETS 16
#endif

// ─── Buffer sizes ──────────────────────────────────────
//
// ## The size follows the PROTOCOL, not the chip
//
// The largest frame the 2.0 model can produce is a text signal: 48
// characters — `a signal carries a value, not a document` — plus the header,
// the address, the type, the tag and the CRC. Fifty-eight bytes. A float is
// fourteen.
//
// The old defaults were sized for something else entirely: widgets with long
// strings and multi-series charts, a class of message that no longer travels.
// On AVR that cost 624 bytes of RAM — measured, on an Uno, 80 % → 61 % — for
// a capacity nothing can ever use.
//
// So the sizes are now multiples of the frame, and the comment says which:
//
//   AVR      RX = 4 frames, TX = 2   — 2 KB of SRAM decides everything
//   others   kept generous            — the RAM is there, and the legacy
//                                       widget decoder is still compiled in
//
// The values below are the DEFAULTS — override before including the lib
// (e.g. `-DINSTANT_RX_BUFFER_SIZE=1024`) for a specific case.

/** The largest frame the signal model can carry, rounded up. */
#ifndef INSTANT_MAX_FRAME_SIZE
    #define INSTANT_MAX_FRAME_SIZE 64
#endif

#ifndef INSTANT_RX_BUFFER_SIZE
    #if defined(INSTANTIOT_PLATFORM_ESP32)
        #define INSTANT_RX_BUFFER_SIZE 2048
    #elif defined(INSTANTIOT_PLATFORM_R4) || defined(INSTANTIOT_PLATFORM_ESP8266)
        #define INSTANT_RX_BUFFER_SIZE 1024
    #else
        // Four whole frames. A frame split across two TCP reads fits, with
        // three more behind it.
        #define INSTANT_RX_BUFFER_SIZE (INSTANT_MAX_FRAME_SIZE * 4)
    #endif
#endif

#ifndef INSTANT_TX_BUFFER_SIZE
    #if defined(INSTANTIOT_PLATFORM_ESP32)
        #define INSTANT_TX_BUFFER_SIZE 1024
    #elif defined(INSTANTIOT_PLATFORM_R4) || defined(INSTANTIOT_PLATFORM_ESP8266)
        #define INSTANT_TX_BUFFER_SIZE 512
    #else
        // Two. A sketch writes one value at a time; the second is the one it
        // writes in the same `loop()` pass.
        #define INSTANT_TX_BUFFER_SIZE (INSTANT_MAX_FRAME_SIZE * 2)
    #endif
#endif

// ============================================================
// 🎛️ ENABLED WIDGETS
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

#ifndef INSTANTIOT_WIDGETS_BARCHART
    #define INSTANTIOT_WIDGETS_BARCHART 1
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
// ============================================================
//  SIGNALS (InstantIoT 2.0)
// ============================================================
// The ceiling a board applies to its own signal frames until the server
// pushes the real one at connection. Same value as the server's fuse, so a
// board that never hears from the server still behaves — and a sketch that
// writes in loop() without a delay can no longer get itself disconnected.
#ifndef INSTANTIOT_DEFAULT_SIGNAL_RATE
#define INSTANTIOT_DEFAULT_SIGNAL_RATE 50
#endif
