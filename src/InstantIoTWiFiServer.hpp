#pragma once
/**
 * ============================================================
 * ☁️  InstantIoTWiFiServer.hpp — Façade mode Server
 * ============================================================
 *
 * Le device ESP32 se connecte en WiFi Station au routeur puis
 * ouvre une session TCP vers un InstantIoT Server distant.
 *
 * Supported platforms:
 *   - ESP32  (autres plateformes à venir)
 *
 * Usage:
 *   #include <InstantIoTWiFiServer.hpp>
 *
 *   InstantIoTWiFiServer instant(
 *       "192.168.1.42",   // IP du serveur
 *       9001,             // port TCP du serveur
 *       "DEVICE_TOKEN"    // token fourni à la création du device
 *   );
 *
 *   void setup() {
 *       Serial.begin(115200);
 *       // Optionnel : change l'intervalle heartbeat (défaut 5000ms).
 *       // Le serveur adapte son timeout de détection offline à ≈ 2.5×
 *       // cette valeur (min 2s, max 2min).
 *       instant.setHeartbeat(5000);
 *       instant.begin("MyWiFi", "MyPassword");  // credentials WiFi
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
#else
    #error "InstantIoTWiFiServer: currently only ESP32 is supported"
#endif

class InstantIoTWiFiServer : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTWiFiServer(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : InstantIoT::InstantIoTCoreBase(_transportImpl)
      , _transportImpl(serverIp, serverPort, token)
    {
        // Heartbeat actif par défaut en mode server (5s).
        // Plumbing les deux couches : l'intervalle est envoyé au
        // serveur dans le handshake (→ soTimeout adaptatif) ET
        // utilisé par `loop()` pour émettre les trames TYPE_HEARTBEAT.
        setHeartbeat(5000);
    }

    // ----- Heartbeat — à appeler avant begin() pour l'annoncer -----
    //
    // Valeurs recommandées : 2000–30000 ms.
    // Serveur clampe à [2s, 120s]. Valeur 0 désactive (mode legacy).
    void setHeartbeat(uint32_t intervalMs) {
        _transportImpl.setHeartbeat(intervalMs);
        InstantIoT::InstantIoTCoreBase::setHeartbeat(intervalMs);
    }

    // ----- Connexion : WiFi puis TCP + handshake -----
    bool begin(const char* ssid, const char* pass) {
        _transportImpl.setCredentials(ssid, pass);
        return InstantIoT::InstantIoTCoreBase::begin();
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
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    InstantIoT::WiFiServerClient_ESP32 _transportImpl;
#endif
};
