#pragma once
/**
 * ============================================================
 * 📡 SoftAP_NINA.hpp — the board IS the network, on a NINA module
 * ============================================================
 *
 * Covers every board whose WiFi is an u-blox NINA-W10 co-processor: MKR
 * WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2. One file, three boards — the
 * `WiFiNINA` library is the same on all of them, and the differences are
 * in the core, not here.
 *
 * ## An access point has no password below eight characters
 *
 * WPA2 refuses shorter ones, and the module answers with a failure that
 * names nothing. The check is here so the message names it.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <WiFiNINA.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

namespace iiot {

class SoftAP_NINA : public ITransport {
public:

    SoftAP_NINA(
        const char* ssid,
        const char* pass,
        uint16_t port = INSTANT_AP_PORT
    ) : ssid_(ssid), pass_(pass), port_(port), server_(port) {}

    bool begin() override {
        IIOT_LOG_VAL("[SoftAP-NINA] Creating: ", ssid_);

        if (WiFi.status() == WL_NO_MODULE) {
            IIOT_LOG("[SoftAP-NINA] No NINA module — check the board and its firmware");
            return false;
        }

        // WPA2 will not take fewer than eight characters, and the module
        // reports that as a plain failure. Say which of the two it is.
        if (pass_ && strlen(pass_) > 0 && strlen(pass_) < 8) {
            IIOT_LOG("[SoftAP-NINA] The password must be 8 characters or more (WPA2)");
            return false;
        }

        int status = WiFi.beginAP(ssid_, pass_);
        if (status != WL_AP_LISTENING) {
            IIOT_LOG("[SoftAP-NINA] FAILED!");
            return false;
        }

        delay(500);
        server_.begin();

        IIOT_LOG("[SoftAP-NINA] Ready");
        return true;
    }

    void poll() override {
        if (client_ && !client_.connected()) {
            client_.stop();
            IIOT_LOG("[SoftAP-NINA] Client disconnected");
        }

        // One client at a time: the board is a dashboard for one phone, and
        // accepting a second would interleave two frame streams on one
        // decoder.
        if (!client_ || !client_.connected()) {
            WiFiClient newClient = server_.available();
            if (newClient) {
                client_ = newClient;
                IIOT_LOG("[SoftAP-NINA] Client connected");
            }
        }
    }

    bool connected() override { return client_ && client_.connected(); }
    int  available() override { return connected() ? client_.available() : 0; }

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return client_.read(buf, len);
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return client_.write(buf, len);
    }

    IPAddress   getIP()   const { return WiFi.localIP(); }
    const char* getSSID() const { return ssid_; }
    uint16_t    getPort() const { return port_; }

private:
    const char*  ssid_;
    const char*  pass_;
    uint16_t     port_;
    WiFiServer   server_;
    WiFiClient   client_;
};

} // namespace iiot
