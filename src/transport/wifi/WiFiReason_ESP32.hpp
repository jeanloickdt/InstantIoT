#pragma once
/**
 * ============================================================
 * 📻 WiFiReason_ESP32.hpp — why the association failed
 * ============================================================
 *
 * ## What this file cost before it existed
 *
 * A board that had always connected stopped doing so. The sketch said
 * "not joined", the library said "WiFi timeout", and it took an hour,
 * four diagnostic sketches and two useless fixes to discover that the
 * password had lost two characters in a copy-paste.
 *
 * The ESP32 knew **from the first second**. It emits a disconnect event
 * carrying a reason code, and that code said `15 — 4WAY_HANDSHAKE_TIMEOUT`,
 * which means "the key is refused". Nobody had asked it.
 *
 * ## Why this one speaks unasked
 *
 * The rest of the library's logging is compiled out while
 * `INSTANTIOT_DEBUG` is 0, and rightly: a chatty log costs flash and time
 * for nothing.
 *
 * A refused association is not "nothing". It is the one moment where the
 * board can do nothing else, and where whoever is watching has no other
 * source of truth. So it speaks — once per reason, not per attempt,
 * otherwise the backoff would turn the monitor into a waterfall.
 *
 * `#define INSTANTIOT_QUIET 1` silences it, for a finished product with
 * other ways to diagnose itself.
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "WiFiReason_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>

namespace iiot {

/** The last reason code received, and the one already reported. */
inline uint8_t& lastWiFiReason() { static uint8_t r = 0; return r; }
inline uint8_t& wifiReasonAlreadyTold() { static uint8_t r = 0; return r; }

/**
 * The reasons actually met in practice, in plain words.
 *
 * The full list lives in `esp_wifi_types.h` and runs to about thirty
 * entries; embedding them all would cost flash for cases nobody sees.
 * These cover what happens on a bench.
 */
inline const char* wifiReasonText(uint8_t r) {
    switch (r) {
        case 15:  return "key refused by the access point (wrong password)";
        case 2:   return "authentication expired";
        case 201: return "access point not found (name, 2.4 GHz band, range)";
        case 202: return "authentication refused (WPA3 only? MAC filtering?)";
        case 203: return "association refused (too many clients?)";
        case 204: return "handshake took too long";
        case 205: return "connection failed";
        default:  return "see esp_wifi_types.h";
    }
}

/** Remembers the reason. Does not print it: an event handler is called
 *  from the WiFi stack, and one does not write to the serial port there. */
inline void rememberWiFiReason(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
        lastWiFiReason() = info.wifi_sta_disconnected.reason;
}

/** Call once, before the first attempt. */
inline void listenForWiFiReasons() {
    static bool registered = false;
    if (registered) return;
    registered = true;
    WiFi.onEvent(rememberWiFiReason);
}

/**
 * States the reason, if it is a new one. Called from `loop()`, never from
 * the event handler.
 */
inline void tellWiFiReason() {
#ifndef INSTANTIOT_QUIET
    uint8_t r = lastWiFiReason();
    if (r == 0 || r == wifiReasonAlreadyTold()) return;
    wifiReasonAlreadyTold() = r;
    Serial.print(F("[InstantIoT] WiFi refused — reason "));
    Serial.print(r);
    Serial.print(F(" : "));
    Serial.println(wifiReasonText(r));
#endif
}

/** After a successful association: the next reason will be a fresh one. */
inline void forgetWiFiReason() {
    lastWiFiReason() = 0;
    wifiReasonAlreadyTold() = 0;
}

} // namespace iiot
