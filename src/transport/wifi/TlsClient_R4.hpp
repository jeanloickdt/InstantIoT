#pragma once
/**
 * ============================================================
 * 🔐 TlsClient_R4.hpp — TLS transport for the Uno R4 WiFi
 * ============================================================
 *
 * The TLS variant for the Uno R4 WiFi. TLS is performed by the on-board
 * ESP32-S3 modem through `WiFiSSLClient` (WiFiS3 library) — so the
 * RA4M1's SRAM is not the wall; the encryption lives on the modem.
 *
 *   - useDefaultCABundle() : use the CA bundle built into the modem
 *   - setCACert(pem) is kept for facade uniformity but is a no-op here.
 *
 * IMPORTANT: pass the **hostname** (not an IP) so that SNI and
 * certificate validation work.
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
      , caCert_(INSTANTIOT_LE_ROOT_X1)   // X1 SEULE : le modem WiFiS3 n'accepte
                                         // qu'un certificat, et X1+X2 concaténées
                                         // échouent chez lui.
                                         //
                                         // X1 n'est atteinte qu'au 4e certificat de
                                         // la chaîne — la X2 croisée par X1, que le
                                         // serveur envoie et qui expire en 2032. Le
                                         // R4 est la seule carte à en dépendre :
                                         // voir `certs/InstantIoT_LE_Roots.h`.
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
    // there is no "insecure" mode. Present for facade uniformity, a
    // façade multi-plateforme.
    void setInsecure() {
        IIOT_LOG("[WiFiR4Sec] setInsecure() ignored — the R4 always validates the certificate");
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
        seedJitter();
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
                IIOT_LOG("[WiFiR4Sec] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                wifiAttemptStartedAt_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiR4Sec] WiFi reconnect attempt #", retryAttempt_);
            startWiFiAttempt();
            return;   // we will look again on the next pass
        }

        wifiAttemptStartedAt_ = 0;

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
        IIOT_LOG_VAL("[WiFiR4Sec] WiFi connecting to: ", ssid_);
        startWiFiAttempt();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // The attempt stays IN FLIGHT: `poll()` will let it finish
                // rather than restart it.
                IIOT_LOG("[WiFiR4Sec] WiFi timeout — the attempt continues in the background");
                return false;
            }
            delay(100);
        }
        // WiFiS3 announces WL_CONNECTED BEFORE DHCP finishes → wait for a
        // valid IP, or TLS starts with no network (localIP == 0.0.0.0).
        while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                IIOT_LOG("[WiFiR4Sec] No DHCP IP (still 0.0.0.0)");
                return false;
            }
            delay(100);
        }
        wifiAttemptStartedAt_ = 0;
        IIOT_LOG_VAL("[WiFiR4Sec] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    bool connectServer() {
        IIOT_LOG_2("[WiFiR4Sec] TLS connecting: ", serverIp_, ":", serverPort_);

        // Trust: embedded root(s), or the modem bundle if nullptr.
        client_.setCACert(caCert_);
        // NB: NO setConnectionTimeout() — a non-zero timeout switches
        // connect() to the _CLIENTCONNECT modem command (temperamental)
        // instead of _CLIENTCONNECTNAME (WiFiS3's reliable path).
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

    // ----- Jitter entropy -----
    //
    // On AVR, SAMD and Renesas, `random()` is a plain PRNG that starts from
    // the same state at every boot: a fleet powered up together produced
    // the same "jitter" and knocked on the relay in the same second. The
    // time the link took to come up is the one thing that differs from
    // board to board. ESP32 and ESP8266 draw from hardware, and calling
    // `randomSeed()` there would DOWNGRADE them to the PRNG.
    void seedJitter() {
#if !defined(ESP32) && !defined(ESP8266)
        uint32_t seed = micros() ^ (millis() << 16);
        randomSeed(seed ? seed : 1);
#endif
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
    /** When the last `WiFi.begin` happened, or 0 if nothing is in flight. */
    uint32_t    wifiAttemptStartedAt_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
