#pragma once
/**
 * ============================================================
 * 📱 InstantIoTBluetooth.hpp - Facade Bluetooth
 * ============================================================
 *
 * Supported platforms:
 *   ESP32                    → Bluetooth Classic built-in (BT_ESP32.hpp)
 *   Uno, Nano, Mega, ESP8266 → SoftwareSerial (BT_Serial.hpp)
 *
 * Usage (ESP32):
 *   InstantIoTBluetooth instant("MyDevice");
 *
 * Usage (Uno / Mega / ESP8266):
 *   InstantIoTBluetooth instant(10, 11);  // RX, TX pins
 *
 * ============================================================
 */

#include "InstantIoTConfig.h"
#include "core/InstantIoTCore.hpp"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/bluetooth/BT_ESP32.hpp"
#else
    #include "transport/bluetooth/BT_Serial.hpp"
#endif

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

// ── ESP32 — Bluetooth Classic built-in ───────────────────────
class InstantIoTBluetooth : public InstantIoT::InstantIoTCoreBase {
public:
    InstantIoTBluetooth(const char* deviceName = "InstantIoT")
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(deviceName)
    {}

private:
    InstantIoT::BT_ESP32 _transportImpl;
};

#else

// ── Uno / Mega / ESP8266 — SoftwareSerial ────────────────────
class InstantIoTBluetooth : public InstantIoT::InstantIoTCoreBase {
public:
    InstantIoTBluetooth(uint8_t rxPin, uint8_t txPin, long baud = INSTANT_BT_BAUDRATE)
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(rxPin, txPin, baud)
    {}

private:
    InstantIoT::BT_Serial _transportImpl;
};

#endif