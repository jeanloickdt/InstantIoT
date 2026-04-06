#pragma once
/**
 * ============================================================
 * 📱 InstantIoTWiFiAP.hpp - Façade WiFi Access Point
 * ============================================================
 *
 * Supported platforms:
 *   - ESP32
 *   - ESP8266
 *   - Arduino Uno R4 WiFi
 *
 * Usage:
 *   #include <InstantIoTWiFiAP.hpp>
 *   InstantIoTWiFiAP instant("MyDevice", "12345678");
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

// Auto-select transport based on platform
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/wifi/SoftAP_ESP32.hpp"
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #include "transport/wifi/SoftAP_ESP8266.hpp"
#elif defined(ARDUINO_UNOWIFIR4)
    #include "transport/wifi/SoftAP_R4.hpp"
#else
    #error "InstantIoTWiFiAP: Unsupported platform (ESP32, ESP8266 or Arduino Uno R4 WiFi required)"
#endif

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

    uint16_t    getPort()  const { return _transportImpl.getPort(); }
    const char* getSSID()  const { return _transportImpl.getSSID(); }
    bool        hasClient()      { return _transportImpl.connected(); }

private:
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    InstantIoT::SoftAP_ESP32 _transportImpl;
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    InstantIoT::SoftAP_ESP8266 _transportImpl;
#elif defined(ARDUINO_UNOWIFIR4)
    InstantIoT::SoftAP_R4 _transportImpl;
#endif
};