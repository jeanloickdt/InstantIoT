#pragma once
/**
 * ============================================================
 * 📡 SoftAP_ESP32.hpp - Transport WiFi SoftAP ESP32
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "SoftAP_ESP32.hpp nécessite une cible ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

#ifndef INSTANT_USE_ACCEPT
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    #define INSTANT_USE_ACCEPT 1
  #else
    #define INSTANT_USE_ACCEPT 0
  #endif
#endif

namespace InstantIoT {

class SoftAP_ESP32 : public ITransport {
public:
    
    SoftAP_ESP32(
        const char* ssid,
        const char* pass,
        uint16_t port = INSTANT_AP_PORT
    ) : ssid_(ssid), pass_(pass), port_(port), server_(port) {}
    
    bool begin() override {
        IIOT_LOG_VAL("[SoftAP] Creating: ", ssid_);
        
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(
            IPAddress(192,168,4,1), 
            IPAddress(192,168,4,1), 
            IPAddress(255,255,255,0)
        );
        
        bool ok = WiFi.softAP(ssid_, pass_, 1, false, 4);
        if (!ok) {
            IIOT_LOG("[SoftAP] FAILED!");
            return false;
        }
        
        delay(500);
        server_.begin();
        server_.setNoDelay(true);
        
        IIOT_LOG_2("[SoftAP] Ready - IP: ", WiFi.softAPIP().toString().c_str(), ":", port_);
        
        return true;
    }
    
    void poll() override {
        if (client_ && !client_.connected()) {
            client_.stop();
            IIOT_LOG("[SoftAP] Client disconnected");
        }
        
#if INSTANT_USE_ACCEPT
        WiFiClient newClient = server_.accept();
#else
        WiFiClient newClient = server_.available();
#endif
        
        if (newClient) {
            if (client_) client_.stop();
            client_ = newClient;
            client_.setNoDelay(true);
            IIOT_LOG("[SoftAP] Client connected");
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
    
    IPAddress getIP() const { return WiFi.softAPIP(); }
    const char* getSSID() const { return ssid_; }
    uint16_t getPort() const { return port_; }
    
private:
    const char* ssid_;
    const char* pass_;
    uint16_t port_;
    WiFiServer server_;
    WiFiClient client_;
};

} // namespace InstantIoT
