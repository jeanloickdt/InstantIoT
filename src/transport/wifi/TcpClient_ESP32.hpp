#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_ESP32.hpp — TCP client transport to InstantIoT Server
 * ============================================================
 *
 * "Server" mode: the ESP32 connects in WiFi Station mode to the router
 * then opens a TCP connection to a remote InstantIoT Server.
 *
 * Handshake: [PAYLOAD_LEN(1B) | PAYLOAD_BYTES]
 *   payload = "token" (legacy) or "token:heartbeatMs" (with heartbeat).
 *   Example: "abc-123-def:5000" (heartbeat 5s).
 * Then: standard iWidgets v1 binary frames.
 *
 * Heartbeat: on the lib side, the **facade** `InstantIoTWiFiServer` calls
 * `setHeartbeat(ms)` on this transport (default 5000ms). The value is
 * sent to the server in the handshake — the server sets its soTimeout to
 * `heartbeat × 2.5` and declares the device offline if nothing is received
 * within that window. The periodic emission of `TYPE_HEARTBEAT` (0xFE)
 * frames is handled in `InstantIoTCoreBase::loop()`.
 *
 * Auto reconnection (WiFi + TCP) with exponential backoff
 * (1s → 2s → 4s → … max 30s).
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "TcpClient_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include "../../core/Transport.h"
#include "RaisonWiFi_ESP32.hpp"
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

// ± jitter applied to the backoff to avoid the "thundering herd":
// if N devices in a fleet restart at the same time (power
// outage, server reboot), without jitter they all retry
// in sync → flood the server. With jitter (±25% by default),
// retries desynchronize naturally.
#ifndef INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT
  #define INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT 25
#endif

namespace iiot {

class TcpClient_ESP32 : public ITransport {
public:

    TcpClient_ESP32(
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
      , heartbeatMs_(0)  // 0 = legacy handshake (no announcement)
    {}

    // ============================================================
    // 💓 Heartbeat — called by the facade before begin()
    // ============================================================
    //
    // Configures the heartbeat interval announced to the server in the
    // handshake. The server adapts its soTimeout (= heartbeat × 2.5).
    // `0` = do not announce (legacy mode, server fallback 90s).
    void setHeartbeat(uint32_t intervalMs) {
        heartbeatMs_ = intervalMs;
    }

    uint32_t getHeartbeat() const { return heartbeatMs_; }

    // ============================================================
    // 🔑 WiFi credentials — called by the facade before begin()
    // ============================================================
    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiServer] Missing WiFi credentials");
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
        // ── Le WiFi n'est pas la ────────────────────────────────
        //
        // Une association peut etre EN COURS. Relancer `WiFi.begin()` a ce
        // moment-la ne la relance pas : elle la TUE et la fait repartir de
        // zero. L'ESP32 le dit lui-meme —
        //
        //     E (86789) wifi:sta is connecting, cannot set config
        //
        // — et une carte sur un reseau lent n'arrive alors jamais : chaque
        // reprise l'interrompt juste avant qu'elle n'aboutisse. La pile
        // continue toute seule ; il suffit de ne plus lui couper la parole.
        if (WiFi.status() != WL_CONNECTED) {
            if (client_) client_.stop();

            // La puce sait POURQUOI, et elle le sait tout de suite : le refus
            // d'une cle arrive en deux secondes. Le dire ici plutot qu'a
            // l'expiration du delai, c'est treize secondes de moins a se
            // demander ce qui se passe. Une fois par raison, pas par essai.
            diLaRaisonWiFi();

            if (tentativeWiFiDepuis_ != 0) {
                // Une tentative est en vol : on regarde, on ne touche pas.
                if (millis() - tentativeWiFiDepuis_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                // Elle a assez dure. On la coupe proprement — sans
                // `disconnect()`, le `begin()` suivant retombe sur la meme
                // erreur — et on laisse le backoff decider du moment.
                IIOT_LOG("[WiFiServer] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                tentativeWiFiDepuis_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiServer] WiFi reconnect attempt #", retryAttempt_);
            lanceLaTentativeWiFi();
            return;   // on rendra la main a la prochaine passe
        }

        // Le WiFi est la : plus rien en vol, et la raison precedente
        // n'a plus cours.
        tentativeWiFiDepuis_ = 0;
        oublieLaRaisonWiFi();

        // TCP dropped → reconnect (with backoff)
        if (!client_.connected()) {
            if (client_) client_.stop();
            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiServer] TCP reconnect attempt #", retryAttempt_);
            if (!connectServer()) {
                scheduleRetry();
                return;
            }
            // SUCCESS: full reset of backoff + counter
            backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
            retryAttempt_ = 0;
        }
    }

    // ============================================================
    // 📡 STATUS
    // ============================================================

    bool connected() override {
        return WiFi.status() == WL_CONNECTED && client_.connected();
    }

    int available() override {
        return connected() ? client_.available() : 0;
    }

    // ============================================================
    // 📥 READ
    // ============================================================

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return client_.read(buf, len);
    }

    // ============================================================
    // 📤 WRITE
    // ============================================================

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return client_.write(buf, len);
    }

    // ============================================================
    // 🔎 GETTERS
    // ============================================================

    IPAddress   getLocalIP()  const { return WiFi.localIP(); }
    const char* getServerIP() const { return serverIp_; }
    uint16_t    getPort()     const { return serverPort_; }
    bool        isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

private:

    /** Le SEUL endroit qui appelle `WiFi.begin`, et il note l'heure. */
    void lanceLaTentativeWiFi() {
        ecouteLesRaisonsWiFi();
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        tentativeWiFiDepuis_ = millis();
    }

    // ----- WiFi -----
    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiServer] WiFi connecting to: ", ssid_);

        lanceLaTentativeWiFi();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // La tentative reste EN VOL : la pile continue d'essayer, et
                // `poll()` la laissera aboutir plutot que de la relancer.
                IIOT_LOG("[WiFiServer] WiFi timeout — la tentative continue en fond");
                return false;
            }
            delay(100);
        }

        tentativeWiFiDepuis_ = 0;
        oublieLaRaisonWiFi();
        IIOT_LOG_VAL("[WiFiServer] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    // ----- TCP + handshake -----
    bool connectServer() {
        IIOT_LOG_2("[WiFiServer] TCP connecting: ", serverIp_, ":", serverPort_);

        client_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);
        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[WiFiServer] TCP connect FAILED");
            return false;
        }
        client_.setNoDelay(true);

        // Handshake: [PAYLOAD_LEN | PAYLOAD_BYTES]
        //   payload = "token"           (legacy, heartbeatMs_ = 0)
        //   payload = "token:heartbeat"  (heartbeat enabled)
        if (!token_) {
            IIOT_LOG("[WiFiServer] Missing device token");
            client_.stop();
            return false;
        }

        // Build the payload (max 255 bytes length-prefixed)
        char payload[288];
        int written = 0;
        if (heartbeatMs_ > 0) {
            written = snprintf(payload, sizeof(payload), "%s:%lu",
                               token_, (unsigned long)heartbeatMs_);
        } else {
            written = snprintf(payload, sizeof(payload), "%s", token_);
        }
        if (written <= 0 || written > 255) {
            IIOT_LOG("[WiFiServer] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[WiFiServer] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[WiFiServer] Handshake sent, heartbeat=", (long)heartbeatMs_);
        return true;
    }

    // ----- Backoff with jitter -----
    //
    // Computes the next retry = backoffMs_ ± jitter%, then doubles
    // backoffMs_ for next time (cap at MAX). Jitter
    // desynchronizes retries across a device fleet (cf. constant
    // INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT).
    //
    // Logs a warning when the MAX cap is reached — a sign of a
    // persistent issue (server down, broken WiFi config, etc.) that
    // the maker should investigate.
    void scheduleRetry() {
        uint32_t base = backoffMs_;
        // Jitter: random() ∈ [-pct, +pct] of base
        // random() is seeded by esp_random() on ESP32 → different
        // per device, hence natural desynchronization of a fleet.
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;  // floor: avoid spin

        nextRetryAt_ = millis() + (uint32_t)actualDelay;

        IIOT_LOG_2("[WiFiServer] Next retry in ", actualDelay, "ms (base ", base);

        // Double for the next, cap at MAX
        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL(
                    "[WiFiServer] Reached max backoff — persistent issue, attempt #",
                    retryAttempt_
                );
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
    uint32_t    tentativeWiFiDepuis_ = 0;  // monotonic counter for debug logs
    uint32_t    heartbeatMs_;       // 0 = legacy, >0 = announced to server
};

} // namespace iiot
