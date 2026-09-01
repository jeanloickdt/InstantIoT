#pragma once
/**
 * ============================================================
 * 🔐 TlsClient_R4.hpp — TLS transport (Arduino Uno R4 WiFi)
 * ============================================================
 *
 * Variante TLS pour l'Uno R4 WiFi (jalon M2). Le TLS est réalisé par
 * le modem ESP32-S3 embarqué via `WiFiSSLClient` (lib WiFiS3) — donc
 * la SRAM du RA4M1 n'est pas le mur, le chiffrement vit sur le modem.
 *
 * Confiance TLS :
 *   - Par défaut : racines Let's Encrypt embarquées via setCACert().
 *   - useDefaultCABundle() : utilise le bundle CA intégré au modem
 *     (setCACert(nullptr)) — utile en secours si l'embarqué échoue.
 *   - ⚠️ PAS de setInsecure() : l'API WiFiSSLClient du R4 valide
 *     toujours l'identité du serveur (pas de mode non vérifié). La
 *     méthode existe pour l'uniformité de la façade mais no-op ici.
 *
 * IMPORTANT : passer le **hostname** (pas une IP) pour le SNI + la
 * vérification du certificat.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_UNOWIFIR4)
#  error "TlsClient_R4.hpp requires Arduino Uno R4 WiFi"
#endif

#include <Arduino.h>
#include <WiFiS3.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"
#include "../../certs/InstantIoT_LE_Roots.h"

#ifndef INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS 15000
#endif

#ifndef INSTANTIOT_TCP_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_TCP_CONNECT_TIMEOUT_MS 5000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MIN_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MIN_MS 1000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MAX_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MAX_MS 30000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT
  #define INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT 25
#endif

namespace iiot {

class TlsClient_R4 : public ITransport {
public:

    TlsClient_R4(
        const char* serverHost,
        uint16_t    serverPort,
        const char* token
    ) : serverIp_(serverHost)
      , serverPort_(serverPort)
      , token_(token)
      , ssid_(nullptr)
      , pass_(nullptr)
      , caCert_(INSTANTIOT_LE_ROOT_X1)   // défaut R4 : X1 SEUL — le modem WiFiS3
                                         // n'accepte qu'un cert ; X1 suffit car la
                                         // chaîne de instantiot.cloud remonte à X1.
                                         // (X1+X2 concaténés échouent sur le modem.)
      , nextRetryAt_(0)
      , backoffMs_(INSTANTIOT_RECONNECT_BACKOFF_MIN_MS)
      , heartbeatMs_(0)
    {}

    void setHeartbeat(uint32_t intervalMs) { heartbeatMs_ = intervalMs; }
    uint32_t getHeartbeat() const { return heartbeatMs_; }

    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ----- Confiance TLS — appeler AVANT begin() -----
    void setCACert(const char* pem)  { caCert_ = pem; }        // racine maison
    void useDefaultCABundle()        { caCert_ = nullptr; }    // bundle du modem

    // ⚠️ No-op sur R4 : WiFiSSLClient valide toujours l'identité, il
    // n'existe pas de mode "insecure". Présent pour l'uniformité de la
    // façade multi-plateforme.
    void setInsecure() {
        IIOT_LOG("[WiFiR4Sec] setInsecure() ignoré — le R4 valide toujours le certificat");
    }

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiR4Sec] Missing WiFi credentials");
            return false;
        }
        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[WiFiR4Sec] WiFi module not found — check the ESP32-S3 firmware");
            return false;
        }
        if (!connectWiFi()) return false;
        if (!connectServer()) return false;
        backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
        return true;
    }

    void poll() override {
        if (WiFi.status() != WL_CONNECTED) {
            client_.stop();
            if (millis() < nextRetryAt_) return;
            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4Sec] WiFi lost — reconnect attempt #", retryAttempt_);
            if (!connectWiFi()) { scheduleRetry(); return; }
        }

        if (!client_.connected()) {
            client_.stop();
            if (millis() < nextRetryAt_) return;
            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4Sec] TLS reconnect attempt #", retryAttempt_);
            if (!connectServer()) { scheduleRetry(); return; }
            backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
            retryAttempt_ = 0;
        }
    }

    bool connected() override {
        return WiFi.status() == WL_CONNECTED && client_.connected();
    }

    int available() override {
        return connected() ? client_.available() : 0;
    }

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return client_.read(buf, len);
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return client_.write(buf, len);
    }

    IPAddress   getLocalIP()  const { return WiFi.localIP(); }
    const char* getServerIP() const { return serverIp_; }
    uint16_t    getPort()     const { return serverPort_; }
    bool        isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

private:

    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiR4Sec] WiFi connecting to: ", ssid_);
        WiFi.begin(ssid_, pass_);

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiR4Sec] WiFi timeout");
                return false;
            }
            delay(100);
        }
        // WiFiS3 annonce WL_CONNECTED AVANT la fin du DHCP → attendre une IP
        // valide, sinon le TLS part sans réseau (localIP == 0.0.0.0).
        while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiR4Sec] No DHCP IP (still 0.0.0.0)");
                return false;
            }
            delay(100);
        }
        IIOT_LOG_VAL("[WiFiR4Sec] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    bool connectServer() {
        IIOT_LOG_2("[WiFiR4Sec] TLS connecting: ", serverIp_, ":", serverPort_);

        // Confiance : racine(s) embarquée(s), ou bundle modem si nullptr.
        client_.setCACert(caCert_);
        // NB : PAS de setConnectionTimeout() — un timeout non nul bascule
        // connect() sur la commande modem _CLIENTCONNECT (capricieuse) au
        // lieu de _CLIENTCONNECTNAME (le chemin fiable de WiFiS3).
        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[WiFiR4Sec] TLS connect FAILED (check CA / port / hostname)");
            return false;
        }

        if (!token_) {
            IIOT_LOG("[WiFiR4Sec] Missing device token");
            client_.stop();
            return false;
        }

        char payload[288];
        int written = 0;
        if (heartbeatMs_ > 0) {
            written = snprintf(payload, sizeof(payload), "%s:%lu",
                               token_, (unsigned long)heartbeatMs_);
        } else {
            written = snprintf(payload, sizeof(payload), "%s", token_);
        }
        if (written <= 0 || written > 255) {
            IIOT_LOG("[WiFiR4Sec] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[WiFiR4Sec] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[WiFiR4Sec] Handshake sent (TLS), heartbeat=", (long)heartbeatMs_);
        return true;
    }

    void scheduleRetry() {
        uint32_t base = backoffMs_;
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;

        nextRetryAt_ = millis() + (uint32_t)actualDelay;
        IIOT_LOG_2("[WiFiR4Sec] Next retry in ", actualDelay, "ms (base ", base);

        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL("[WiFiR4Sec] Reached max backoff — persistent issue, attempt #", retryAttempt_);
            }
            next = INSTANTIOT_RECONNECT_BACKOFF_MAX_MS;
        }
        backoffMs_ = next;
    }

    const char* serverIp_;
    uint16_t    serverPort_;
    const char* token_;
    const char* ssid_;
    const char* pass_;
    const char* caCert_;

    WiFiSSLClient client_;
    uint32_t    nextRetryAt_;
    uint32_t    backoffMs_;
    uint32_t    retryAttempt_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
