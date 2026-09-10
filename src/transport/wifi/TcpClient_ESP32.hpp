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
 * Then: standard binary frames.
 *
 * Heartbeat: the facade calls `setHeartbeat(ms)` before `begin()`. The value
 * is announced to the server in the handshake — the server sets its soTimeout
 * to `heartbeat × 2.5` and declares the device offline if nothing arrives
 * within that window. The periodic `TYPE_HEARTBEAT` (0xFE) emission lives in
 * `InstantIoTCoreBase::loop()`.
 *
 * ## What is left here
 *
 * Almost nothing, and that is the point. Session, handshake, retry, backoff
 * with jitter and the in-flight rule are in [TcpSession] — they were the same
 * code as the Uno R4's, word for word. This file answers two questions —
 * *is the WiFi up?*, *bring it up* — and adds the one thing only the ESP32
 * can do: say WHY an association failed.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "TcpClient_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include "../TcpSession.hpp"
#include "WiFiReason_ESP32.hpp"

namespace iiot {

class TcpClient_ESP32 : public TcpSession {
public:

    TcpClient_ESP32(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    // `wifiClient_` is handed to the base before it is itself constructed —
    // members are built after bases. That is legal here, and only here,
    // because [TcpSession] merely STORES the reference: it never touches the
    // client during construction. Its storage exists from the start; only its
    // constructor runs later.
    ) : TcpSession(wifiClient_, serverIp, serverPort, token)
      , ssid_(nullptr)
      , pass_(nullptr)
    {}

    // ============================================================
    // 🔑 WiFi credentials — called by the facade before begin()
    // ============================================================
    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ============================================================
    // 🔎 GETTERS
    // ============================================================

    IPAddress getLocalIP()        const { return WiFi.localIP(); }
    bool      isWiFiConnected()   const { return WiFi.status() == WL_CONNECTED; }

protected:

    bool linkUp() const override { return WiFi.status() == WL_CONNECTED; }

    /**
     * The ONLY place that calls `WiFi.begin`.
     *
     * An association may be IN FLIGHT. Calling `WiFi.begin()` at that moment
     * does not restart it: it KILLS it and starts from zero. The ESP32 says
     * so itself —
     *
     *     E (86789) wifi:sta is connecting, cannot set config
     *
     * — and a board on a slow network then never arrives: every retry
     * interrupts the attempt just before it completes.
     *
     * [TcpSession] is what keeps that from happening: it only calls this
     * when nothing is in flight. The rule is written down there; this
     * comment is the reason it exists.
     */
    bool beginLink() override {
        listenForWiFiReasons();
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        return true;
    }

    /** Without `disconnect()` the next `begin()` hits the same error. */
    void endLinkAttempt() override { WiFi.disconnect(); }

    /** The ESP32 wants both; the R4 wants neither. See [TcpSession::prepareClient]. */
    void prepareClient() override {
        wifiClient_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);
        wifiClient_.setNoDelay(true);
    }

    /**
     * Keepalive: a dead server is noticed in about 30 s, not in minutes.
     * Probes start after 15 s of silence, every 5 s, 3 misses close.
     */
    void tuneSession() override {
        int on = 1, idle = 15, intv = 5, cnt = 3;
        wifiClient_.setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
        wifiClient_.setOption(TCP_KEEPIDLE,  &idle);
        wifiClient_.setOption(TCP_KEEPINTVL, &intv);
        wifiClient_.setOption(TCP_KEEPCNT,   &cnt);
    }

    bool linkCredentialsReady() const override {
        if (ssid_ && pass_) return true;
        IIOT_LOG("[WiFiServer] Missing WiFi credentials");
        return false;
    }

    /**
     * The chip knows WHY, and knows at once: a refused key comes back in two
     * seconds. Saying it here rather than at the timeout is thirteen seconds
     * less spent wondering. Once per reason, never once per retry.
     */
    void onLinkDown() override { tellWiFiReason(); }
    void onLinkUp()   override { forgetWiFiReason(); }

private:
    const char* ssid_;
    const char* pass_;
    WiFiClient  wifiClient_;
};

} // namespace iiot
