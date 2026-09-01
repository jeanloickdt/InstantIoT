#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_R4.hpp — plaintext TCP transport for the Uno R4 WiFi
 * ============================================================
 *
 * The equivalent of TcpClient_ESP32 for the Uno R4 WiFi. The R4 uses the
 * `WiFiS3` library (WiFi handled by the on-board ESP32-S3 modem), with
 * two differences from the ESP32:
 *   - no setNoDelay(); the timeout is set with setConnectionTimeout(ms)
 *   - WL_CONNECTED is announced BEFORE DHCP finishes, so we wait for a
 *     valid IP.
 *
 * PLAINTEXT — for LAN, self-hosting or a test. For the cloud over the
 * internet, prefer the TLS variant (TlsClient_R4).
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
        // With no credentials there is nothing to retry — and
        // `WiFi.begin(nullptr)` is unforgiving. The guard used to live in
        // `begin()` alone; now that `loop()` runs `poll()` even after a
        // failed `begin()`, it must be here too.
        if (!ssid_ || !pass_) return;
        // ── The network is not ready ────────────────────────────
        //
        // Same rule as on ESP32: an association may be IN FLIGHT, and
        // restarting it kills it instead of helping. Watch, do not touch,
        // until the window has passed.
        //
        // "Ready" means an IP, not just `WL_CONNECTED`: WiFiS3 announces
        // the connection BEFORE DHCP finishes, and going to TLS with
        // 0.0.0.0 fails without explaining anything.
        if (!networkReady()) {
            client_.stop();

            if (wifiAttemptStartedAt_ != 0) {
                if (millis() - wifiAttemptStartedAt_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                IIOT_LOG("[WiFiR4] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                wifiAttemptStartedAt_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4] WiFi reconnect attempt #", retryAttempt_);
            startWiFiAttempt();
            return;   // we will look again on the next pass
        }

        wifiAttemptStartedAt_ = 0;

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

    /** An IP, not just `WL_CONNECTED` — see `poll()`. */
    bool networkReady() const {
        return WiFi.status() == WL_CONNECTED
            && WiFi.localIP() != IPAddress(0, 0, 0, 0);
    }

    /** The ONLY place that calls `WiFi.begin`, and it records when. */
    void startWiFiAttempt() {
        WiFi.begin(ssid_, pass_);
        wifiAttemptStartedAt_ = millis();
    }

    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiR4] WiFi connecting to: ", ssid_);
        startWiFiAttempt();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // The attempt stays IN FLIGHT: `poll()` will let it finish
                // rather than restart it.
                IIOT_LOG("[WiFiR4] WiFi timeout — the attempt continues in the background");
                return false;
            }
            delay(100);
        }
        // WiFiS3 announces WL_CONNECTED BEFORE DHCP finishes → wait for a
        // valid IP, or the server connection starts with no network.
        while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiR4] No DHCP IP (still 0.0.0.0)");
                return false;
            }
            delay(100);
        }
        wifiAttemptStartedAt_ = 0;
        IIOT_LOG_VAL("[WiFiR4] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    bool connectServer() {
        IIOT_LOG_2("[WiFiR4] TCP connecting: ", serverIp_, ":", serverPort_);

        // NB : on n'appelle PAS setConnectionTimeout() — un timeout non nul
        // switches WiFiClient::connect() to the _CLIENTCONNECT modem
        // command (temperamental) instead of _CLIENTCONNECTNAME, WiFiS3's
        // reliable standard path. Leave the default (0).
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
    /** When the last `WiFi.begin` happened, or 0 if nothing is in flight. */
    uint32_t    wifiAttemptStartedAt_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
