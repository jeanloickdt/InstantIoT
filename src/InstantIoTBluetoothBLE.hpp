#pragma once
/**
 * ============================================================
 * 📱 InstantIoTBluetoothBLE.hpp - Facade BLE ESP32
 * ============================================================
 *
 * Uses NimBLE-Arduino library (lightweight BLE stack).
 * Install via Arduino Library Manager: "NimBLE-Arduino"
 *
 * Compatible boards:
 *   ESP32, ESP32-S2, ESP32-S3, ESP32-C3
 *
 * Usage:
 *   #include <InstantIoTBluetoothBLE.hpp>
 *   InstantIoTBluetoothBLE instant("MyDevice");
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * Android connects via BLE scan — no pairing required.
 *
 * ============================================================
 */

#if !defined(ESP32)
    #error "InstantIoTBluetoothBLE: ESP32 family only"
#endif

#include "InstantIoTConfig.h"
#include "core/InstantIoTCore.hpp"
#include "transport/bluetooth/BT_ESP32_BLE.hpp"

class InstantIoTBluetoothBLE : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTBluetoothBLE(const char* deviceName = "InstantIoT")
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(deviceName)
    {}

private:
    InstantIoT::BT_ESP32_BLE _transportImpl;
};