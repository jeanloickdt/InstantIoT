#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_R4.hpp — plaintext TCP transport for the Uno R4 WiFi
 * ============================================================
 *
 * The equivalent of TcpClient_ESP32 for the Uno R4 WiFi. The R4 uses the
 * `WiFiS3` library (WiFi handled by the on-board ESP32-S3 modem), and it
 * differs from the ESP32 in three ways — all three live in this file, and
 * nowhere else:
 *
 *   1. **No timeout on the client, ever.** A non-zero `setConnectionTimeout()`
 *      switches `WiFiClient::connect()` to the `_CLIENTCONNECT` modem command,
 *      which is temperamental, instead of `_CLIENTCONNECTNAME`, WiFiS3's
 *      reliable standard path. The default (0) is the working one.
 *   2. **`WL_CONNECTED` arrives BEFORE DHCP.** Going to the server with
 *      `0.0.0.0` fails without explaining anything, so "the link is up" means
 *      *and we have an IP*.
 *   3. **No event API.** WiFiS3 cannot say WHY an association failed, so the
 *      two reason hooks stay empty here — the ESP32 overrides them.
 *
 * Everything else — session, handshake, retry, backoff, the in-flight rule —
 * is in [TcpSession], shared with the ESP32.
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
#include "../TcpSession.hpp"

namespace iiot {

class TcpClient_R4 : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    TcpClient_R4(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : TcpSession(wifiClient_, serverIp, serverPort, token)
      , ssid_(nullptr)
      , pass_(nullptr)
    {}

    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    IPAddress getLocalIP()      const { return WiFi.localIP(); }
    bool      isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

protected:

    /**
     * An IP, not just `WL_CONNECTED`.
     *
     * WiFiS3 announces the connection before DHCP finishes. Reporting the
     * link up at that moment sends the session to the server with `0.0.0.0`,
     * and the failure says nothing about DHCP.
     */
    bool linkUp() const override {
        return WiFi.status() == WL_CONNECTED
            && WiFi.localIP() != IPAddress(0, 0, 0, 0);
    }

    /** The ONLY place that calls `WiFi.begin` — [TcpSession] guards it. */
    bool beginLink() override {
        WiFi.begin(ssid_, pass_);
        return true;
    }

    /** Without `disconnect()` the next `begin()` hits the same error. */
    void endLinkAttempt() override { WiFi.disconnect(); }

    /**
     * Nothing. Deliberately.
     *
     * This is the one place where the empty default of
     * [TcpSession::prepareClient] is the whole point: setting a timeout here
     * would move `connect()` onto the temperamental modem command. See the
     * header of this file.
     */
    void prepareClient() override {}

    bool linkCredentialsReady() const override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiR4] Missing WiFi credentials");
            return false;
        }
        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[WiFiR4] WiFi module not found — check the ESP32-S3 firmware");
            return false;
        }
        return true;
    }

private:
    const char* ssid_;
    const char* pass_;
    WiFiClient  wifiClient_;
};

} // namespace iiot
