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
 *   #include <InstantIoTBluetoothESP32SPP.hpp>
 *   InstantIoTBluetoothESP32SPP instant("MyDevice");
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * Android pairs with the device name via Bluetooth Classic SPP.
 *
 * ============================================================
 */

#if defined(ESP32)

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class BT_ESP32 : public ITransport {
public:

    BT_ESP32(const char* deviceName)
        : _deviceName(deviceName)
    {}

    bool begin() override {
        IIOT_LOG("[BT-ESP32] Starting...");
        IIOT_LOG_VAL("[BT-ESP32] Device name: ", _deviceName);
        if (!_bt.begin(_deviceName)) {
            IIOT_LOG("[BT-ESP32] Failed to start Bluetooth!");
            return false;
        }
        IIOT_LOG("[BT-ESP32] Ready — waiting for connection");
        return true;
    }

    void poll() override {}

    bool connected() override { return _bt.connected(); }
    int  available() override { return _bt.available(); }

    int read(uint8_t* buf, size_t len) override {
        size_t count = 0;
        while (count < len && _bt.available() > 0) {
            buf[count++] = (uint8_t)_bt.read();
        }
        return count > 0 ? (int)count : -1;
    }

    size_t write(const uint8_t* buf, size_t len) override {
        return _bt.write(buf, len);
    }

private:
    BluetoothSerial _bt;
    const char*     _deviceName;
};

} // namespace InstantIoT

#endif // ESP32