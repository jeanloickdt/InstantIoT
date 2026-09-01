#pragma once
/**
 * ============================================================
 * ☁️  InstantIoTWiFiServer.hpp — Server mode facade
 * ============================================================
 *
 * The ESP32 device connects in WiFi Station mode to the router then
 * opens a TCP session to a remote InstantIoT Server.
 *
 * Supported platforms:
 *   - ESP32
 *   - Arduino Uno R4 WiFi (via WiFiS3)
 *
 * Usage:
 *   #include <InstantIoTWiFiServer.hpp>
 *
 *   InstantIoTWiFiServer instant(
 *       "192.168.1.42",   // server IP
 *       9001,             // server TCP port
 *       "DEVICE_TOKEN"    // token provided when the device was created
 *   );
 *
 *   void setup() {
 *       Serial.begin(115200);
 *       // Optional: change heartbeat interval (default 5000ms).
 *       // The server adapts its offline detection timeout to ≈ 2.5×
 *       // this value (min 2s, max 2min).
 *       instant.setHeartbeat(5000);
 *       instant.begin("MyWiFi", "MyPassword");  // WiFi credentials
 *   }
 *
 *   void loop() {
 *       instant.loop();
 *   }
 *
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/wifi/WiFiServerClient_ESP32.hpp"
    namespace iiot { using InstantWiFiServerTransport = WiFiServerClient_ESP32; }
#elif defined(ARDUINO_UNOWIFIR4)
    #include "transport/wifi/WiFiServerClient_R4.hpp"
    namespace iiot { using InstantWiFiServerTransport = WiFiServerClient_R4; }
#else
    #error "InstantIoTWiFiServer: supported on ESP32 and Arduino Uno R4 WiFi"
#endif

class InstantIoTWiFiServer : public iiot::InstantIoTCoreBase {
public:

    InstantIoTWiFiServer(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : iiot::InstantIoTCoreBase(_transportImpl)
      , _transportImpl(serverIp, serverPort, token)
    {
        // Heartbeat active by default in server mode (5s).
        // Plumbing both layers: the interval is sent to the
        // server in the handshake (→ adaptive soTimeout) AND
        // used by `loop()` to emit TYPE_HEARTBEAT frames.
        setHeartbeat(5000);
    }

    // ----- Heartbeat — call before begin() to announce it -----
    //
    // Recommended values: 2000–30000 ms.
    // Server clamps to [2s, 120s]. Value 0 disables (legacy mode).
    void setHeartbeat(uint32_t intervalMs) {
        _transportImpl.setHeartbeat(intervalMs);
        iiot::InstantIoTCoreBase::setHeartbeat(intervalMs);
    }

    // ----- Connection: WiFi then TCP + handshake -----
    bool begin(const char* ssid, const char* pass) {
        _transportImpl.setCredentials(ssid, pass);
        return iiot::InstantIoTCoreBase::begin();
    }

    // ----- Getters -----
    const char* getLocalIP() {
        static char ipStr[16];
        IPAddress ip = _transportImpl.getLocalIP();
        snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        return ipStr;
    }

    const char* getServerIP()     const { return _transportImpl.getServerIP(); }
    uint16_t    getPort()         const { return _transportImpl.getPort(); }
    bool        isWiFiConnected() const { return _transportImpl.isWiFiConnected(); }

private:
    iiot::InstantWiFiServerTransport _transportImpl;
};
