#pragma once
/**
 * ============================================================
 * 📡 BT_Serial.hpp - Transport Bluetooth via SoftwareSerial
 * ============================================================
 *
 * Compatible with any Bluetooth Serial module:
 *   HC-05, HC-06, HM-10, JDY-30, etc.
 *
 * Supported boards:
 *   Arduino Uno, Nano, Mega, ESP8266
 *
 * Wiring:
 *   Module RX → TX_PIN (use voltage divider if 5V board)
 *   Module TX → RX_PIN
 *   Module VCC → 3.3V or 5V (check module specs)
 *   Module GND → GND
 *
 * Note: SoftwareSerial stable up to ~57600 baud.
 *
 * ============================================================
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_BT_BAUDRATE
    #define INSTANT_BT_BAUDRATE 9600
#endif

#ifndef INSTANT_BT_TIMEOUT_MS
    #define INSTANT_BT_TIMEOUT_MS 5000
#endif

namespace InstantIoT {

class BT_Serial : public ITransport {
public:

    BT_Serial(uint8_t rxPin, uint8_t txPin, long baudrate = INSTANT_BT_BAUDRATE)
        : _serial(rxPin, txPin)
        , _baudrate(baudrate)
        , _connected(false)
        , _lastRxMs(0)
    {}

    // ── Lifecycle ─────────────────────────────────────────────

    bool begin() override {
        IIOT_LOG("[BT-Serial] Starting...");
        IIOT_LOG_VAL("[BT-Serial] Baudrate: ", _baudrate);
        _serial.begin(_baudrate);
        _connected = false;
        _lastRxMs  = 0;
        IIOT_LOG("[BT-Serial] Ready — waiting for connection");
        return true;
    }

    void poll() override {
        if (_serial.available() > 0) {
            if (!_connected) {
                _connected = true;
                IIOT_LOG("[BT-Serial] Device connected");
            }
            _lastRxMs = millis();
        }
        if (_connected && (millis() - _lastRxMs > INSTANT_BT_TIMEOUT_MS)) {
            _connected = false;
            IIOT_LOG("[BT-Serial] Device disconnected (timeout)");
        }
    }

    // ── Status ────────────────────────────────────────────────

    bool connected() override { return true; }
    int  available() override { return _serial.available(); }

    // ── Read ──────────────────────────────────────────────────

    int read(uint8_t* buf, size_t len) override {
        size_t count = 0;
        while (count < len && _serial.available() > 0) {
            buf[count++] = (uint8_t)_serial.read();
        }
        return count > 0 ? (int)count : -1;
    }

    // ── Write ─────────────────────────────────────────────────

    size_t write(const uint8_t* buf, size_t len) override {
        return _serial.write(buf, len);
    }

private:
    SoftwareSerial _serial;
    long           _baudrate;
    bool           _connected;
    uint32_t       _lastRxMs;
};

} // namespace InstantIoT