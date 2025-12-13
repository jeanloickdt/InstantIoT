#pragma once
/**
 * ============================================================
 * 📡 SoftAP_ESP8266.hpp - Transport WiFi SoftAP ESP8266
 * ============================================================
 */

#if defined(ESP8266)

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

namespace InstantIoT {

class SoftAP_ESP8266 : public ITransport {
public:

    SoftAP_ESP8266(
        const char* ssid,
        const char* password,
        uint16_t port = INSTANT_AP_PORT
    ) : _ssid(ssid), _password(password), _port(port), _ip(192, 168, 4, 1), _server(port) {}

    bool begin() override {
        IIOT_LOG("[SoftAP-8266] Creating AP...");
        IIOT_LOG_VAL("[SoftAP-8266] SSID: ", _ssid);

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(_ip, _ip, IPAddress(255, 255, 255, 0));

        if (!WiFi.softAP(_ssid, _password)) {
            IIOT_LOG("[SoftAP-8266] Failed to create AP!");
            return false;
        }

        delay(500);
        _server.begin();
        _server.setNoDelay(true);

        IIOT_LOG("[SoftAP-8266] AP Ready!");
        IIOT_LOG_VAL("[SoftAP-8266] IP: ", WiFi.softAPIP().toString().c_str());
        IIOT_LOG_VAL("[SoftAP-8266] Port: ", _port);

        return true;
    }

    void poll() override {
        // Vérifier si client déconnecté
        if (_client && !_client.connected()) {
            _client.stop();
            IIOT_LOG("[SoftAP-8266] Client disconnected");
        }

        // Accepter nouveau client
        WiFiClient newClient = _server.accept();
        if (newClient) {
            if (_client) _client.stop();
            _client = newClient;
            _client.setNoDelay(true);
            IIOT_LOG("[SoftAP-8266] Client connected");
        }
    }

    bool connected() override {
        return _client && _client.connected();
    }

    int available() override {
        return connected() ? _client.available() : 0;
    }

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return _client.read(buf, len);
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return _client.write(buf, len);
    }

    // Info
    IPAddress getIP() const { return WiFi.softAPIP(); }
    const char* getSSID() const { return _ssid; }
    uint16_t getPort() const { return _port; }

private:
    const char* _ssid;
    const char* _password;
    uint16_t _port;
    IPAddress _ip;
    WiFiServer _server;
    WiFiClient _client;
};

} // namespace InstantIoT

#endif // ESP8266