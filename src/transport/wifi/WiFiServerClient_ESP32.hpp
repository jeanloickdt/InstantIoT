#pragma once
/**
 * ============================================================
 * 🌐 WiFiServerClient_ESP32.hpp — TCP client transport to InstantIoT Server
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
#  error "WiFiServerClient_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
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

// ± jitter applied to the backoff to avoid the "thundering herd":
// if N devices in a fleet restart at the same time (power
// outage, server reboot), without jitter they all retry
// in sync → flood the server. With jitter (±25% by default),
// retries desynchronize naturally.
#ifndef INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT
  #define INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT 25
#endif

namespace InstantIoT {

class WiFiServerClient_ESP32 : public ITransport {
public:

    WiFiServerClient_ESP32(
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
        // WiFi dropped → reconnect (with backoff)
        if (WiFi.status() != WL_CONNECTED) {
            if (client_) client_.stop();
            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiServer] WiFi lost — reconnect attempt #", retryAttempt_);
            if (!connectWiFi()) {
                scheduleRetry();
                return;
            }
            // WiFi restored — reset the attempt count, we try TCP
            // in the next block. TCP backoff uses its own
            // sequence (the shared counter is OK for V1).
        }

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

    // ----- WiFi -----
    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiServer] WiFi connecting to: ", ssid_);

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiServer] WiFi timeout");
                return false;
            }
            delay(100);
        }

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
    uint32_t    retryAttempt_ = 0;  // monotonic counter for debug logs
    uint32_t    heartbeatMs_;       // 0 = legacy, >0 = announced to server
};

} // namespace InstantIoT
