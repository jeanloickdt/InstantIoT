#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * InstantIoTDebug.hpp - Système de debug portable
 * 
 * Niveaux:
 * 0 = OFF (aucun log)
 * 1 = ERROR
 * 2 = WARN
 * 3 = INFO
 * 4 = DEBUG
 * 5 = VERBOSE
 * 
 * Usage:
 *   INSTANTIOT_LOG_INFO("Connected to %s", ssid);
 *   INSTANTIOT_LOG_ERROR("Failed with code %d", errorCode);
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>
#include "../InstantIoTConfig.h"

// ============================================================
// 🎨 COULEURS ANSI (pour terminaux supportés)
// ============================================================

#ifdef INSTANTIOT_DEBUG_COLORS
    #define INSTANTIOT_COLOR_RESET   "\033[0m"
    #define INSTANTIOT_COLOR_RED     "\033[31m"
    #define INSTANTIOT_COLOR_YELLOW  "\033[33m"
    #define INSTANTIOT_COLOR_GREEN   "\033[32m"
    #define INSTANTIOT_COLOR_BLUE    "\033[34m"
    #define INSTANTIOT_COLOR_GRAY    "\033[90m"
#else
    #define INSTANTIOT_COLOR_RESET   ""
    #define INSTANTIOT_COLOR_RED     ""
    #define INSTANTIOT_COLOR_YELLOW  ""
    #define INSTANTIOT_COLOR_GREEN   ""
    #define INSTANTIOT_COLOR_BLUE    ""
    #define INSTANTIOT_COLOR_GRAY    ""
#endif

// ============================================================
// 📝 MACROS DE LOG
// ============================================================

#if INSTANTIOT_DEBUG_LEVEL >= 1
    #define INSTANTIOT_LOG_ERROR(fmt, ...) \
        do { \
            INSTANTIOT_DEBUG_SERIAL.print(INSTANTIOT_COLOR_RED "[ERROR] " INSTANTIOT_COLOR_RESET); \
            INSTANTIOT_DEBUG_SERIAL.printf(fmt "\n", ##__VA_ARGS__); \
        } while(0)
#else
    #define INSTANTIOT_LOG_ERROR(fmt, ...) ((void)0)
#endif

#if INSTANTIOT_DEBUG_LEVEL >= 2
    #define INSTANTIOT_LOG_WARN(fmt, ...) \
        do { \
            INSTANTIOT_DEBUG_SERIAL.print(INSTANTIOT_COLOR_YELLOW "[WARN]  " INSTANTIOT_COLOR_RESET); \
            INSTANTIOT_DEBUG_SERIAL.printf(fmt "\n", ##__VA_ARGS__); \
        } while(0)
#else
    #define INSTANTIOT_LOG_WARN(fmt, ...) ((void)0)
#endif

#if INSTANTIOT_DEBUG_LEVEL >= 3
    #define INSTANTIOT_LOG_INFO(fmt, ...) \
        do { \
            INSTANTIOT_DEBUG_SERIAL.print(INSTANTIOT_COLOR_GREEN "[INFO]  " INSTANTIOT_COLOR_RESET); \
            INSTANTIOT_DEBUG_SERIAL.printf(fmt "\n", ##__VA_ARGS__); \
        } while(0)
#else
    #define INSTANTIOT_LOG_INFO(fmt, ...) ((void)0)
#endif

#if INSTANTIOT_DEBUG_LEVEL >= 4
    #define INSTANTIOT_LOG_DEBUG(fmt, ...) \
        do { \
            INSTANTIOT_DEBUG_SERIAL.print(INSTANTIOT_COLOR_BLUE "[DEBUG] " INSTANTIOT_COLOR_RESET); \
            INSTANTIOT_DEBUG_SERIAL.printf(fmt "\n", ##__VA_ARGS__); \
        } while(0)
#else
    #define INSTANTIOT_LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if INSTANTIOT_DEBUG_LEVEL >= 5
    #define INSTANTIOT_LOG_VERBOSE(fmt, ...) \
        do { \
            INSTANTIOT_DEBUG_SERIAL.print(INSTANTIOT_COLOR_GRAY "[VERB]  " INSTANTIOT_COLOR_RESET); \
            INSTANTIOT_DEBUG_SERIAL.printf(fmt "\n", ##__VA_ARGS__); \
        } while(0)
#else
    #define INSTANTIOT_LOG_VERBOSE(fmt, ...) ((void)0)
#endif

// ============================================================
// 🔧 HELPERS
// ============================================================

// Log hexdump (niveau VERBOSE)
#if INSTANTIOT_DEBUG_LEVEL >= 5
    inline void INSTANTIOT_LOG_HEXDUMP(const char* label, const uint8_t* data, size_t len) {
        INSTANTIOT_DEBUG_SERIAL.printf("[VERB]  %s (%d bytes): ", label, len);
        for (size_t i = 0; i < len && i < 64; i++) {
            INSTANTIOT_DEBUG_SERIAL.printf("%02X ", data[i]);
        }
        if (len > 64) INSTANTIOT_DEBUG_SERIAL.print("...");
        INSTANTIOT_DEBUG_SERIAL.println();
    }
#else
    #define INSTANTIOT_LOG_HEXDUMP(label, data, len) ((void)0)
#endif

// Assert avec message
#ifdef INSTANTIOT_DEBUG
    #define INSTANTIOT_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                INSTANTIOT_LOG_ERROR("ASSERT FAILED: %s at %s:%d", msg, __FILE__, __LINE__); \
                while(1) { delay(1000); } \
            } \
        } while(0)
#else
    #define INSTANTIOT_ASSERT(cond, msg) ((void)0)
#endif

// Mesure de temps
#ifdef INSTANTIOT_DEBUG
    #define INSTANTIOT_TIME_START(name) unsigned long _timer_##name = micros()
    #define INSTANTIOT_TIME_END(name) \
        do { \
            unsigned long _elapsed = micros() - _timer_##name; \
            INSTANTIOT_LOG_DEBUG("%s took %lu us", #name, _elapsed); \
        } while(0)
#else
    #define INSTANTIOT_TIME_START(name) ((void)0)
    #define INSTANTIOT_TIME_END(name) ((void)0)
#endif
