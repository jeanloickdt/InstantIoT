#pragma once
/**
 * ============================================================
 * 📱 InstantIoTBluetooth.hpp - Facade Bluetooth Serial
 * ============================================================
 *
 * Supported platforms:
 *   Uno, Nano, Mega, ESP8266 → SoftwareSerial (BT_Serial.hpp)
 *
 * Usage:
 *   #include <InstantIoTBluetooth.hpp>
 *
 *   // RX_PIN = pin connected to BT module TX
 *   // TX_PIN = pin connected to BT module RX
 *   InstantIoTBluetooth instant(10, 11);
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * ============================================================
 */

#include "InstantIoTConfig.h"
#include "core/InstantIoTCore.hpp"
#include "transport/bluetooth/BT_Serial.hpp"

class InstantIoTBluetooth : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTBluetooth(uint8_t rxPin, uint8_t txPin, long baud = INSTANT_BT_BAUDRATE)
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(rxPin, txPin, baud)
    {}

private:
    InstantIoT::BT_Serial _transportImpl;
};