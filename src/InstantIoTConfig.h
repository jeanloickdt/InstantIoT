#pragma once
/**
 * ============================================================
 * ⚙️ InstantIoTConfig.h - Global configuration
 * ============================================================
 *
 * Supported platforms — the ones `test/boards.sh` actually compiles:
 *   - ESP32 (esp32, S2, S3, C3, C6)
 *   - ESP8266
 *   - Arduino Uno R4 WiFi
 *   - MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2  (u-blox NINA-W10)
 *   - AVR — Mega and up, over Ethernet or Serial
 *
 * ============================================================
 */

// ============================================================
// 🔍 AUTOMATIC PLATFORM DETECTION
// ============================================================
//
// ## The list is the bench, not the ambition
//
// A platform is named here when `test/boards.sh` compiles it. Anything else
// still builds — nothing below is load-bearing — but it gets the warning,
// and the warning has to stay true: it told the owner of a MKR 1010 that
// their board was unofficial for a whole stage after it had been ported.
//
// ## Exact board macros for the NINA three, architecture for the rest
//
// The three NINA boards are named one by one because that is what the port
// covers: another SAMD with no radio has never been compiled here, and it
// should still hear the warning.

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    #define INSTANTIOT_PLATFORM_ESP32
#elif defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
    #define INSTANTIOT_PLATFORM_ESP8266
#elif defined(ARDUINO_UNOWIFIR4)
    #define INSTANTIOT_PLATFORM_R4
#elif defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
    // SAMD21: 32 KB of SRAM, the same room as the R4.
    #define INSTANTIOT_PLATFORM_SAMD
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
    // Same radio as the two above, a very different chip underneath:
    // ATmega4809, 6 KB of SRAM. It belongs with the AVRs for everything
    // that costs memory, and that is the only reason it is a separate name.
    #define INSTANTIOT_PLATFORM_MEGAAVR
#elif defined(ARDUINO_ARCH_AVR)
    // No radio at all — Ethernet or Serial. 8 KB on a Mega, 2 on an Uno.
    #define INSTANTIOT_PLATFORM_AVR
#else
    #warning "InstantIoT: untested platform. Compiled and measured on ESP32, ESP8266, Uno R4 WiFi, MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2 and AVR."
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

/**
 * La taille des quatre chaines nommees que la carte transporte.
 *
 * Le nom vient de l'avant-2.0 et ne decrit plus ce qu'il dimensionne : le nom
 * de la carte, son identifiant, son tableau de bord, et le creneau WID du
 * codec historique. Il est garde tel quel parce qu'un croquis a pu le
 * redefinir — le renommer casserait ce croquis pour ranger un mot.
 */
#ifndef INSTANTIOT_MAX_WIDGET_ID_LENGTH
    #define INSTANTIOT_MAX_WIDGET_ID_LENGTH 32
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
// The dividing line is SRAM, not the radio. The Uno WiFi Rev.2 has the same
// WiFi module as a MKR 1010 and 6 KB against its 32: it sits with the AVRs
// here, and with the NINA boards in `Links.hpp`. Two different questions,
// two different groupings — sorting it by radio in both places would have
// handed an ATmega4809 the ration of a chip five times its size.
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
    #elif defined(INSTANTIOT_PLATFORM_R4) \
       || defined(INSTANTIOT_PLATFORM_ESP8266) \
       || defined(INSTANTIOT_PLATFORM_SAMD)
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
    #elif defined(INSTANTIOT_PLATFORM_R4) \
       || defined(INSTANTIOT_PLATFORM_ESP8266) \
       || defined(INSTANTIOT_PLATFORM_SAMD)
        #define INSTANT_TX_BUFFER_SIZE 512
    #else
        // Two. A sketch writes one value at a time; the second is the one it
        // writes in the same `loop()` pass.
        #define INSTANT_TX_BUFFER_SIZE (INSTANT_MAX_FRAME_SIZE * 2)
    #endif
#endif

// ============================================================
// 🎛️ ENABLED WIDGETS — le decodeur historique, et lui seul
// ============================================================
//
// Ces seize drapeaux ne sont PAS morts : chacun garde un `case` de
// `BinaryCodec::decodePayload`, le decodeur de l'avant-2.0. Les eteindre
// retire vraiment du code du binaire.
//
// Ce qui est mort, c'est le chemin qui y mene. `BinaryCodec::decode()` n'a
// aucun appelant dans `src/` — son unique appelant du depot est une ligne de
// `test/host/test_signals.cpp`. Une carte 2.0 recoit des trames SIGNAL, que
// `decodeSignal` lit, et ne passe jamais ici.
//
// Ils restent donc, et la dette est nommee dans ARCHITECTURE.md §15 : c'est
// le decodeur qu'il faudra decider de retirer, pas ses interrupteurs.

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
