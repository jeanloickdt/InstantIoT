#pragma once
/**
 * ============================================================
 * 🔒 TlsClient_NINA.hpp — TLS over a NINA module
 * ============================================================
 *
 * `WiFiSSLClient` instead of `WiFiClient`, and nothing else changes: it
 * derives from the same Arduino `Client`, so [TcpSession] holds it without
 * knowing. The handshake happens inside `connect()`, in the co-processor.
 *
 * ## ⚠️ THE RESERVATION — read this before promising a customer anything
 *
 * **The trust store is in the NINA firmware, and a sketch cannot add to it.**
 *
 * On an ESP32 you pass your own root with `setCACert(pem)`. Here there is no
 * such call — none: `WiFiSSLClient` exposes `connect()` and nothing about
 * certificates. The roots were burned into the module when its firmware was
 * flashed, and the only way to change them is to reflash the module through
 * the Arduino IDE's *WiFi101 / WiFiNINA Firmware Updater*, which uploads a
 * chosen list of roots — a manual, per-board, out-of-band operation.
 *
 * So there are exactly two outcomes, and no third:
 *
 *   1. **The root of your server is already in the module** — Let's Encrypt's
 *      ISRG Root X1 is, in recent firmware — and TLS works with no setup.
 *   2. **It is not** — a private CA, a self-signed certificate — and the
 *      handshake fails. The answer is either the firmware updater, or
 *      `.plaintext()`.
 *
 * `Cloud(TOKEN).withCertificate(MY_ROOT)` therefore cannot be honoured on
 * this board. [setCACert] keeps the name — the facade is the same across
 * platforms — and SAYS it is ignoring the call, once, at the moment it is
 * made. The same choice the Uno R4 made for `setInsecure()`, for the same
 * reason: silently accepting a root that will not be used is worse than a
 * line in the log.
 *
 * This is written here so it is not discovered twice.
 *
 * ## Old firmware fails the same way as a wrong clock
 *
 * A certificate has dates, and a module that has never been updated may have
 * neither the current roots nor a plausible notion of time. Both produce the
 * same silence. If a handshake fails on a board that used to work, the
 * firmware updater is the first thing to try, not the last.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <WiFiNINA.h>
#include <WiFiSSLClient.h>
#include "../TcpSession.hpp"

namespace iiot {

class TlsClient_NINA : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    TlsClient_NINA(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : TcpSession(sslClient_, serverIp, serverPort, token)
      , ssid_(nullptr)
      , pass_(nullptr)
    {}

    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ----- TLS trust — call BEFORE begin() -----

    /**
     * ⚠️ Ignored on this board, and it says so.
     *
     * There is no API to add a root: the store is in the NINA firmware. See
     * the header of this file for the two outcomes and the way out.
     */
    void setCACert(const char*) {
        IIOT_LOG("[NINA-TLS] setCACert() ignored — the roots live in the NINA "
                 "firmware and a sketch cannot add one. Either the server's root "
                 "is already there, or use .plaintext().");
    }

    /**
     * ⚠️ Ignored too. `WiFiSSLClient` always validates; there is no
     * "insecure" mode to switch to. Same as the Uno R4.
     */
    void setInsecure() {
        IIOT_LOG("[NINA-TLS] setInsecure() ignored — this module always validates");
    }

    IPAddress getLocalIP()      const { return WiFi.localIP(); }
    bool      isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

protected:

    bool linkUp() const override { return WiFi.status() == WL_CONNECTED; }

    bool beginLink() override {
        WiFi.begin(ssid_, pass_);
        return true;
    }

    void endLinkAttempt() override { WiFi.disconnect(); }

    bool linkCredentialsReady() const override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[NINA-TLS] Missing WiFi credentials");
            return false;
        }
        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[NINA-TLS] No NINA module — check the board and its firmware");
            return false;
        }
        return true;
    }

private:
    const char*    ssid_;
    const char*    pass_;
    WiFiSSLClient  sslClient_;
};

} // namespace iiot
