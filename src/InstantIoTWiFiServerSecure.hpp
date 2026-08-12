#pragma once
/**
 * ============================================================
 * 🔐 InstantIoTWiFiServerSecure.hpp — Cloud mode (TLS) facade
 * ============================================================
 *
 * Variante **TLS** de InstantIoTWiFiServer : l'ESP32 se connecte en
 * WiFi Station puis ouvre une session **chiffrée** vers un serveur
 * InstantIoT Cloud. Le token et les données ne transitent jamais en
 * clair sur internet (jalon M2).
 *
 * Plateformes : ESP32 et Arduino Uno R4 WiFi. Les deux font le TLS ;
 * sur le R4 il est déporté sur le modem ESP32-S3 embarqué.
 *
 * Usage :
 *   #include <InstantIoTWiFiServerSecure.hpp>
 *
 *   // Port 9443 par défaut (portier TLS du cloud InstantIoT).
 *   InstantIoTWiFiServerSecure instant(
 *       "instantiot.cloud",   // hostname du serveur (SNI + validation)
 *       "DEVICE_TOKEN"        // token du device
 *   );
 *
 *   void setup() {
 *       Serial.begin(115200);
 *       instant.begin("MyWiFi", "MyPassword");
 *   }
 *   void loop() { instant.loop(); }
 *
 * Confiance TLS (défaut : racines Let's Encrypt embarquées) :
 *   instant.setCACert(myPem);  // serveur self-hosted avec cert maison
 *   instant.setInsecure();     // ⚠️ bring-up/débogage : pas de vérif d'identité
 *
 * ⚠️ SERVER_IP doit être le **hostname** (pas une IP brute) pour que
 * la validation du certificat et le SNI fonctionnent.
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/wifi/WiFiServerClientSecure_ESP32.hpp"
    namespace InstantIoT { using InstantWiFiSecureTransport = WiFiServerClientSecure_ESP32; }
#elif defined(ARDUINO_UNOWIFIR4)
    #include "transport/wifi/WiFiServerClientSecure_R4.hpp"
    namespace InstantIoT { using InstantWiFiSecureTransport = WiFiServerClientSecure_R4; }
#else
    #error "InstantIoTWiFiServerSecure: TLS cloud mode is supported on ESP32 and Arduino Uno R4 WiFi"
#endif

// Port TLS device par défaut du cloud InstantIoT (portier caddy-l4).
#ifndef INSTANTIOT_CLOUD_TLS_PORT
  #define INSTANTIOT_CLOUD_TLS_PORT 9443
#endif

class InstantIoTWiFiServerSecure : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTWiFiServerSecure(
        const char* serverHost,
        const char* token,
        uint16_t    serverPort = INSTANTIOT_CLOUD_TLS_PORT
    ) : InstantIoT::InstantIoTCoreBase(_transportImpl)
      , _transportImpl(serverHost, serverPort, token)
    {
        // Heartbeat actif par défaut (5s), plombé sur les deux couches.
        setHeartbeat(5000);
    }

    // ----- Heartbeat — appeler avant begin() -----
    void setHeartbeat(uint32_t intervalMs) {
        _transportImpl.setHeartbeat(intervalMs);
        InstantIoT::InstantIoTCoreBase::setHeartbeat(intervalMs);
    }

    // ----- Confiance TLS — appeler avant begin() -----
    void setCACert(const char* pem) { _transportImpl.setCACert(pem); }
    void setInsecure()              { _transportImpl.setInsecure(); }

    // ----- Connexion : WiFi puis TLS + handshake -----
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
    InstantIoT::InstantWiFiSecureTransport _transportImpl;
};
