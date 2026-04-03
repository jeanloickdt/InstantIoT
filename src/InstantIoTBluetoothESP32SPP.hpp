#pragma once
/**
 * ============================================================
 * 📱 InstantIoTBluetoothESP32SPP.hpp - Facade Bluetooth Classic ESP32
 * ============================================================
 *
 * Uses ESP32 built-in Bluetooth Classic (SPP profile).
 * No external module required.
 *
 * Board: ESP32 only
 *
 * Usage:
 *   #include <InstantIoTBluetoothESP32SPP.hpp>
 *   InstantIoTBluetoothESP32SPP instant("MyDevice");
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * Android pairs with the device name via Bluetooth Classic.
 *
 * ============================================================
 */

#if defined(ESP32)

#include "InstantIoTConfig.h"
#include "core/InstantIoTCore.hpp"
#include "transport/bluetooth/BT_ESP32.hpp"

class InstantIoTBluetoothESP32SPP : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTBluetoothESP32SPP(const char* deviceName = "InstantIoT")
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(deviceName)
    {}

private:
    InstantIoT::BT_ESP32 _transportImpl;
};

#endif // ESP32