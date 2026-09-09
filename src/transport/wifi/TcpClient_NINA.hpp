#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_NINA.hpp — plaintext TCP over a NINA module
 * ============================================================
 *
 * MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2 — three boards, one file,
 * because their WiFi is the same u-blox NINA-W10 co-processor driven by the
 * same `WiFiNINA` library.
 *
 * Session, handshake, retry and the in-flight rule come from [TcpSession].
 *
 * ## `setNoDelay` does not exist here, and that is fine
 *
 * The ESP32 turns Nagle off because it can. The NINA library exposes no such
 * control: the TCP stack lives in the co-processor's firmware, and the sketch
 * talks to it over SPI. The frames are small and sent one at a time, so what
 * Nagle would coalesce, we were not sending anyway.
 *
 * PLAINTEXT — for LAN or a server you host. For the cloud, `TlsClient_NINA`.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <WiFiNINA.h>
#include "../TcpSession.hpp"

namespace iiot {

class TcpClient_NINA : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    TcpClient_NINA(
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

    bool linkUp() const override { return WiFi.status() == WL_CONNECTED; }

    /**
     * The ONLY place that calls `WiFi.begin` — [TcpSession] guards it.
     *
     * True: the module negotiates on its own side of the SPI bus, so there
     * is something in flight to watch rather than restart.
     */
    bool beginLink() override {
        WiFi.begin(ssid_, pass_);
        return true;
    }

    /** Without `disconnect()` the next `begin()` hits the same error. */
    void endLinkAttempt() override { WiFi.disconnect(); }

    bool linkCredentialsReady() const override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[NINA] Missing WiFi credentials");
            return false;
        }
        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[NINA] No NINA module — check the board and its firmware");
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
