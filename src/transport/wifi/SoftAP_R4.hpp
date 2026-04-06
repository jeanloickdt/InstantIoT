#pragma once
/**
 * ============================================================
 * 📡 SoftAP_R4.hpp - Transport WiFi SoftAP Arduino Uno R4 WiFi
 * ============================================================
 */

#if !defined(ARDUINO_UNOWIFIR4)
#  error "SoftAP_R4.hpp requires Arduino Uno R4 WiFi"
#endif

#include <Arduino.h>
#include <WiFiS3.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

namespace InstantIoT {

class SoftAP_R4 : public ITransport {
public:

    SoftAP_R4(
        const char* ssid,
        const char* pass,
        uint16_t port = INSTANT_AP_PORT
    ) : ssid_(ssid), pass_(pass), port_(port), server_(port) {}

    bool begin() override {
        IIOT_LOG_VAL("[SoftAP-R4] Creating: ", ssid_);

        int status = WiFi.beginAP(ssid_, pass_);
        if (status != WL_AP_LISTENING) {
            IIOT_LOG("[SoftAP-R4] FAILED!");
            return false;
        }

        delay(500);
        server_.begin();

        IIOT_LOG("[SoftAP-R4] Ready");
        return true;
    }

    void poll() override {
        if (client_ && !client_.connected()) {
            client_.stop();
            IIOT_LOG("[SoftAP-R4] Client disconnected");
        }

        // Only accept new client if none connected
        if (!client_ || !client_.connected()) {
            WiFiClient newClient = server_.available();
            if (newClient) {
                client_ = newClient;
                IIOT_LOG("[SoftAP-R4] Client connected");
            }
        }
    }

    bool connected() override {
        return client_ && client_.connected();
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

    IPAddress getIP() const { return WiFi.localIP(); }
    const char* getSSID() const { return ssid_; }
    uint16_t getPort() const { return port_; }

private:
    const char*  ssid_;
    const char*  pass_;
    uint16_t     port_;
    WiFiServer   server_;
    WiFiClient   client_;
};

} // namespace InstantIoT