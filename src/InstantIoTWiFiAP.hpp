#pragma once
/**
 * ============================================================
 * 📡 InstantIoTWiFiAP.hpp - Façade WiFi Access Point
 * ============================================================
 *
 * Supported platforms:
 *   - ESP32
 *   - ESP8266
 *
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

// Auto-select transport selon plateforme
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/wifi/SoftAP_ESP32.hpp"
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #include "transport/wifi/SoftAP_ESP8266.hpp"
#else
    #error "InstantIoTWiFiAP: Plateforme non supportée (ESP32 ou ESP8266 requis)"
#endif

/**
 * Façade WiFi Access Point
 */
class InstantIoTWiFiAP : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTWiFiAP(
        const char* ssid,
        const char* password,
        uint16_t port = 8080
    ) : InstantIoT::InstantIoTCoreBase(_transportImpl)
      , _transportImpl(ssid, password, port)
    {}

    const char* getIP() const {
        static char ipStr[16];
        IPAddress ip = _transportImpl.getIP();
        snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        return ipStr;
    }

    uint16_t getPort() const { return _transportImpl.getPort(); }
    const char* getSSID() const { return _transportImpl.getSSID(); }
    bool hasClient() { return _transportImpl.connected(); }

private:
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    InstantIoT::SoftAP_ESP32 _transportImpl;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    InstantIoT::SoftAP_ESP8266 _transportImpl;
#endif
};