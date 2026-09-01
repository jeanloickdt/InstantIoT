#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_R4.hpp — TCP client transport (Arduino Uno R4 WiFi)
 * ============================================================
 *
 * Équivalent de TcpClient_ESP32 pour l'Uno R4 WiFi. Le R4
 * utilise la lib `WiFiS3` (WiFi géré par le modem ESP32-S3 embarqué),
 * donc quelques différences avec l'ESP32 :
 *   - include <WiFiS3.h> (pas <WiFi.h>)
 *   - pas de WiFi.mode(WIFI_STA) — WiFi.begin() connecte en station
 *   - pas de setNoDelay() ; le timeout se règle via setConnectionTimeout(ms)
 *
 * Protocole identique : handshake [LEN | "token:heartbeatMs"] puis
 * frames binaires iWidgets v1. Backoff exponentiel + jitter.
 *
 * PLAINTEXT — pour le LAN/selfhost ou un test. Pour le cloud sur
 * internet, préférer la variante TLS (TlsClient_R4).
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_UNOWIFIR4)
#  error "TcpClient_R4.hpp requires Arduino Uno R4 WiFi"
#endif

#include <Arduino.h>
#include <WiFiS3.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

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

class TcpClient_R4 : public ITransport {
public:

    TcpClient_R4(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : serverIp_(serverIp)
      , serverPort_(serverPort)
      , token_(token)
      , ssid_(nullptr)
      , pass_(nullptr)
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

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiR4] Missing WiFi credentials");
            return false;
        }
        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[WiFiR4] WiFi module not found — check the ESP32-S3 firmware");
            return false;
        }
        if (!connectWiFi()) return false;
        if (!connectServer()) return false;
        backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
        return true;
    }

    void poll() override {
        // Sans identifiants, il n'y a rien a retenter — et `WiFi.begin(nullptr)`
        // ne pardonne pas. Le garde-fou vivait dans `begin()` seul ; depuis que
        // `loop()` fait tourner `poll()` meme apres un `begin()` rate, il doit
        // etre ici aussi.
        if (!ssid_ || !pass_) return;
        // ── Le reseau n'est pas pret ────────────────────────────
        //
        // Meme regle que sur ESP32 : une association peut etre EN COURS, et
        // la relancer la tue au lieu de l'aider. On regarde, on ne touche
        // pas, jusqu'a ce que le delai soit ecoule.
        //
        // « Pret » veut dire une IP, pas seulement `WL_CONNECTED` : WiFiS3
        // annonce la connexion AVANT la fin du DHCP, et partir en TLS avec
        // 0.0.0.0 echoue sans rien expliquer.
        if (!reseauPret()) {
            client_.stop();

            if (tentativeWiFiDepuis_ != 0) {
                if (millis() - tentativeWiFiDepuis_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                IIOT_LOG("[WiFiR4] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                tentativeWiFiDepuis_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4] WiFi reconnect attempt #", retryAttempt_);
            lanceLaTentativeWiFi();
            return;   // on rendra la main a la prochaine passe
        }

        tentativeWiFiDepuis_ = 0;

        if (!client_.connected()) {
            client_.stop();
            if (millis() < nextRetryAt_) return;
            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4] TCP reconnect attempt #", retryAttempt_);
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

    /** Une IP, et pas seulement `WL_CONNECTED` — voir `poll()`. */
    bool reseauPret() const {
        return WiFi.status() == WL_CONNECTED
            && WiFi.localIP() != IPAddress(0, 0, 0, 0);
    }

    /** Le SEUL endroit qui appelle `WiFi.begin`, et il note l'heure. */
    void lanceLaTentativeWiFi() {
        WiFi.begin(ssid_, pass_);
        tentativeWiFiDepuis_ = millis();
    }

    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiR4] WiFi connecting to: ", ssid_);
        lanceLaTentativeWiFi();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // La tentative reste EN VOL : `poll()` la laissera aboutir
                // plutot que de la relancer.
                IIOT_LOG("[WiFiR4] WiFi timeout — la tentative continue en fond");
                return false;
            }
            delay(100);
        }
        // WiFiS3 annonce WL_CONNECTED AVANT la fin du DHCP → attendre une IP
        // valide, sinon la connexion serveur part sans réseau (0.0.0.0).
        while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiR4] No DHCP IP (still 0.0.0.0)");
                return false;
            }
            delay(100);
        }
        tentativeWiFiDepuis_ = 0;
        IIOT_LOG_VAL("[WiFiR4] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    bool connectServer() {
        IIOT_LOG_2("[WiFiR4] TCP connecting: ", serverIp_, ":", serverPort_);

        // NB : on n'appelle PAS setConnectionTimeout() — un timeout non nul
        // fait basculer WiFiClient::connect() sur la commande modem
        // _CLIENTCONNECT (capricieuse) au lieu de _CLIENTCONNECTNAME, le
        // chemin standard fiable de WiFiS3. Laisser le défaut (0).
        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[WiFiR4] TCP connect FAILED");
            return false;
        }

        if (!token_) {
            IIOT_LOG("[WiFiR4] Missing device token");
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
            IIOT_LOG("[WiFiR4] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[WiFiR4] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[WiFiR4] Handshake sent, heartbeat=", (long)heartbeatMs_);
        return true;
    }

    void scheduleRetry() {
        uint32_t base = backoffMs_;
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;

        nextRetryAt_ = millis() + (uint32_t)actualDelay;
        IIOT_LOG_2("[WiFiR4] Next retry in ", actualDelay, "ms (base ", base);

        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL("[WiFiR4] Reached max backoff — persistent issue, attempt #", retryAttempt_);
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

    WiFiClient  client_;
    uint32_t    nextRetryAt_;
    uint32_t    backoffMs_;
    uint32_t    retryAttempt_ = 0;
    /** Heure du dernier `WiFi.begin`, ou 0 si rien n'est en vol. */
    uint32_t    tentativeWiFiDepuis_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
