#pragma once
/**
 * ============================================================
 * 🌐 TcpClient_ESP8266.hpp — plaintext TCP transport for the ESP8266
 * ============================================================
 *
 * The ESP8266 could only ever be an access point: the board WAS the network,
 * the phone joined it, and there was no way to reach a server. This file is
 * the other direction — join an existing WiFi, then open a TCP session.
 *
 * ## What is left here
 *
 * Two questions. Session, handshake, retry, backoff and the in-flight rule
 * are in [TcpSession], written once for every TCP link — that is what makes
 * this file forty lines instead of three hundred and sixty.
 *
 * ## PLAINTEXT, and that is the whole file
 *
 * The token travels readable. On a LAN, in front of a server you host, that
 * is a decision. Across the internet it is a risk, and the answer is
 * `TlsClient_ESP8266` — which now exists, and cost 104 KB of flash and about
 * 20 KB of heap per session to get there.
 *
 * This file remains the right one for a server on your own network: those
 * 20 KB are heap the sketch does not get back while a session is open, and
 * encryption on a LAN protects a journey that never leaves the house.
 *
 * ## Why it looks like the ESP32 one and is not shared with it
 *
 * `WiFi.status()`, `WiFi.begin()`, `WiFi.disconnect()`: the two APIs are
 * spelled the same because the ESP8266 core was there first and the ESP32
 * core followed it. What differs is underneath — no `listenForWiFiReasons`
 * here, because the ESP8266's event API does not report a disconnect reason
 * the way the ESP32's does.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP8266) && !defined(ESP8266)
#  error "TcpClient_ESP8266.hpp requires ESP8266"
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../TcpSession.hpp"

namespace iiot {

class TcpClient_ESP8266 : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    TcpClient_ESP8266(
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
     * True: the association negotiates in the background, so there IS
     * something in flight to watch. Restarting it would kill it, which is
     * the rule the trunk enforces.
     */
    bool beginLink() override {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        return true;
    }

    /** Without `disconnect()` the next `begin()` hits the same error. */
    void endLinkAttempt() override { WiFi.disconnect(); }

    /** Same as the ESP32: a timeout and no Nagle on a control channel. */
    void prepareClient() override {
        wifiClient_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);
        wifiClient_.setNoDelay(true);
    }

    bool linkCredentialsReady() const override {
        if (ssid_ && pass_) return true;
        IIOT_LOG("[WiFi8266] Missing WiFi credentials");
        return false;
    }

private:
    const char* ssid_;
    const char* pass_;
    WiFiClient  wifiClient_;
};

} // namespace iiot
