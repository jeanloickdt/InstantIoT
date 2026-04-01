#pragma once
/**
 * ============================================================
 * 📡 BT_ESP32.hpp - Transport Bluetooth Classic ESP32
 * ============================================================
 *
 * Uses ESP32 built-in Bluetooth Classic (no external module).
 *
 * Board : ESP32 only
 *
 * Usage:
 *   #include <InstantIoTBluetooth.hpp>
 *   InstantIoTBluetooth instant("MyDevice");
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * Android pairs with the device name via Bluetooth Classic SPP.
 *
 * ============================================================
 */


#include <Arduino.h>
#include <BluetoothSerial.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_BT_TIMEOUT_MS
    #define INSTANT_BT_TIMEOUT_MS 5000
#endif

namespace InstantIoT {

class BT_ESP32 : public ITransport {
public:

    BT_ESP32(const char* deviceName)
        : _deviceName(deviceName)
        , _connected(false)
        , _lastRxMs(0)
    {}

    // ── Lifecycle ─────────────────────────────────────────────

    bool begin() override {
        IIOT_LOG("[BT-ESP32] Starting...");
        IIOT_LOG_VAL("[BT-ESP32] Device name: ", _deviceName);

        if (!_bt.begin(_deviceName)) {
            IIOT_LOG("[BT-ESP32] Failed to start Bluetooth!");
            return false;
        }

        _connected = false;
        _lastRxMs  = 0;

        IIOT_LOG("[BT-ESP32] Ready — waiting for connection");
        return true;
    }

    void poll() override {
        if (_bt.available() > 0) {
            if (!_connected) {
                _connected = true;
                IIOT_LOG("[BT-ESP32] Device connected");
            }
            _lastRxMs = millis();
        }
        if (_connected && (millis() - _lastRxMs > INSTANT_BT_TIMEOUT_MS)) {
            _connected = false;
            IIOT_LOG("[BT-ESP32] Device disconnected (timeout)");
        }
    }

    // ── Status ────────────────────────────────────────────────

    bool connected() override { return true; }
    int  available() override { return _bt.available(); }

    // ── Read ──────────────────────────────────────────────────

    int read(uint8_t* buf, size_t len) override {
        size_t count = 0;
        while (count < len && _bt.available() > 0) {
            buf[count++] = (uint8_t)_bt.read();
        }
        return count > 0 ? (int)count : -1;
    }

    // ── Write ─────────────────────────────────────────────────

    size_t write(const uint8_t* buf, size_t len) override {
        return _bt.write(buf, len);
    }

private:
    BluetoothSerial _bt;
    const char*     _deviceName;
    bool            _connected;
    uint32_t        _lastRxMs;
};

} // namespace InstantIoT
