#pragma once
/**
 * ============================================================
 * 🔐 TlsClient_ESP32.hpp — TLS transport to InstantIoT Cloud
 * ============================================================
 *
 * The **TLS** variant of TcpClient_ESP32: the ESP32 joins a WiFi network
 * as a station, then opens an **encrypted (TLS)** connection to an
 * InstantIoT cloud server. The token and every frame travel encrypted —
 * never readable on the internet.
 *
 * Identical to the plaintext transport for everything else:
 *   Handshake: [PAYLOAD_LEN(1B) | "token" | "token:heartbeatMs"]
 *   then binary frames. Heartbeat driven by the facade, exponential
 *   backoff with jitter.
 *
 * Server verification (default): the Let's Encrypt roots (ISRG Root X1
 * and X2) are embedded, so the ESP32 checks the server's identity. For
 * hardware bring-up or debugging:
 *   - setCACert(pem) : supply your own root (self-hosted server).
 *   - setInsecure()  : encrypt WITHOUT verifying identity (not for
 *                      production — MITM is possible).
 *
 * On the server side, a TLS gateway (caddy-l4) terminates TLS on port
 * 9443 and forwards to the relay internally.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "TlsClient_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <lwip/sockets.h>
#include "../../core/Transport.h"
#include "WiFiReason_ESP32.hpp"
#include "../../InstantIoTConfig.h"
#include "../../certs/InstantIoT_LE_Roots.h"

#ifndef INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS 15000
#endif

#ifndef INSTANTIOT_TCP_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_TCP_CONNECT_TIMEOUT_MS 5000
#endif

// TLS handshake: bounds the negotiation time (seconds) so a silent
// connection cannot block the device's boot.
#ifndef INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S
  #define INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S 10
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

class TlsClient_ESP32 : public ITransport {
public:

    TlsClient_ESP32(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : serverIp_(serverIp)
      , serverPort_(serverPort)
      , token_(token)
      , ssid_(nullptr)
      , pass_(nullptr)
      , caCert_(INSTANTIOT_LE_ROOT_CAS)  // validation par défaut (Let's Encrypt)
      , insecure_(false)
      , nextRetryAt_(0)
      , backoffMs_(INSTANTIOT_RECONNECT_BACKOFF_MIN_MS)
      , heartbeatMs_(0)
    {}

    // ============================================================
    // 💓 Heartbeat — called by the facade before begin()
    // ============================================================
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
    // 🔐 TLS trust — call BEFORE begin()
    // ============================================================
    //
    // setCACert: supply your own root (e.g. a self-hosted server with
    //   its own certificate). Replaces the Let's Encrypt roots.
    void setCACert(const char* pem) {
        caCert_   = pem;
        insecure_ = false;
    }

    // setInsecure: encrypts WITHOUT verifying the server's identity.
    //   ⚠️ MITM possible — bring-up/debugging only, not production.
    void setInsecure() {
        insecure_ = true;
    }

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiSecure] Missing WiFi credentials");
            return false;
        }

        if (!connectWiFi()) return false;
        if (!connectServer()) return false;

        backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
        return true;
    }

    void poll() override {
        // With no credentials there is nothing to retry — and
        // `WiFi.begin(nullptr)` is unforgiving. The guard used to live in
        // `begin()` alone; now that `loop()` runs `poll()` even after a
        // failed `begin()`, it must be here too.
        if (!ssid_ || !pass_) return;

        // ── WiFi is not there ───────────────────────────────────
        //
        // An association may be IN FLIGHT. Calling `WiFi.begin()` at that
        // moment does not restart it: it KILLS it and starts from zero.
        // The ESP32 says so itself —
        //
        //     E (86789) wifi:sta is connecting, cannot set config
        //
        // — and a board on a slow network then never arrives: every retry
        // interrupts the attempt just before it completes. The stack keeps
        // trying on its own; we only have to stop cutting it off.
        if (WiFi.status() != WL_CONNECTED) {
            client_.stop();

            // The chip knows WHY, and knows at once: a refused key comes
            // back in two seconds. Saying it here rather than at the
            // timeout is thirteen seconds less spent wondering. Once per
            // reason, not per attempt.
            tellWiFiReason();

            if (wifiAttemptStartedAt_ != 0) {
                // An attempt is in flight: watch, do not touch.
                if (millis() - wifiAttemptStartedAt_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                // It has lasted long enough. Cut it cleanly — without
                // `disconnect()` the next `begin()` hits the same error —
                // and let the backoff decide when to try again.
                IIOT_LOG("[WiFiSecure] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                wifiAttemptStartedAt_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiSecure] WiFi reconnect attempt #", retryAttempt_);
            startWiFiAttempt();
            return;   // we will look again on the next pass
        }

        // WiFi is up: nothing in flight, and the previous reason no
        // longer applies.
        wifiAttemptStartedAt_ = 0;
        forgetWiFiReason();

        // TLS/TCP tombé → reconnexion (avec backoff)
        if (!client_.connected()) {
            client_.stop();
            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiSecure] TLS reconnect attempt #", retryAttempt_);
            if (!connectServer()) {
                scheduleRetry();
                return;
            }
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

    /** The ONLY place that calls `WiFi.begin`, and it records when. */
    void startWiFiAttempt() {
        listenForWiFiReasons();
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        wifiAttemptStartedAt_ = millis();
    }

    // ----- WiFi -----
    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiSecure] WiFi connecting to: ", ssid_);

        startWiFiAttempt();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // The attempt stays IN FLIGHT: the stack keeps trying, and
                // `poll()` will let it finish rather than restart it.
                IIOT_LOG("[WiFiSecure] WiFi timeout — the attempt continues in the background");
                return false;
            }
            delay(100);
        }

        wifiAttemptStartedAt_ = 0;
        forgetWiFiReason();
        IIOT_LOG_VAL("[WiFiSecure] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    // ----- TLS + TCP + handshake -----
    bool connectServer() {
        IIOT_LOG_2("[WiFiSecure] TLS connecting: ", serverIp_, ":", serverPort_);

        // Confiance TLS — configurée AVANT connect() (chaque tentative).
        if (insecure_) {
            client_.setInsecure();
            IIOT_LOG("[WiFiSecure] ⚠️ INSECURE mode — server identity NOT verified");
        } else {
            client_.setCACert(caCert_);
        }
        client_.setHandshakeTimeout(INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S);
        client_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);

        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[WiFiSecure] TLS connect FAILED (check CA / port / SNI)");
            return false;
        }
        tuneSession();

        // Handshake applicatif : [PAYLOAD_LEN | PAYLOAD_BYTES]
        //   payload = "token"           (legacy, heartbeatMs_ = 0)
        //   payload = "token:heartbeat"  (heartbeat activé)
        if (!token_) {
            IIOT_LOG("[WiFiSecure] Missing device token");
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
            IIOT_LOG("[WiFiSecure] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[WiFiSecure] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[WiFiSecure] Handshake sent (TLS), heartbeat=", (long)heartbeatMs_);
        return true;
    }

    // ----- Backoff avec jitter (identique au transport clair) -----
    /**
     * Keepalive: a dead server is noticed in about 30 s, not in minutes.
     * Probes start after 15 s of silence, every 5 s, 3 misses close.
     */
    void tuneSession() {
        int on = 1, idle = 15, intv = 5, cnt = 3;
        client_.setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
        client_.setOption(TCP_KEEPIDLE,  &idle);
        client_.setOption(TCP_KEEPINTVL, &intv);
        client_.setOption(TCP_KEEPCNT,   &cnt);
    }

    void scheduleRetry() {
        uint32_t base = backoffMs_;
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;

        nextRetryAt_ = millis() + (uint32_t)actualDelay;

        IIOT_LOG_2("[WiFiSecure] Next retry in ", actualDelay, "ms (base ", base);

        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL(
                    "[WiFiSecure] Reached max backoff — persistent issue, attempt #",
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
    const char* caCert_;            // racine(s) de confiance (PEM)
    bool        insecure_;          // true = chiffre sans vérifier l'identité

    WiFiClientSecure client_;
    uint32_t    nextRetryAt_;
    uint32_t    backoffMs_;
    uint32_t    retryAttempt_ = 0;
    /** When the last `WiFi.begin` happened, or 0 if nothing is in flight. */
    uint32_t    wifiAttemptStartedAt_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
