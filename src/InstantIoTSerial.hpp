#pragma once
/**
 * ============================================================
 * 📱 InstantIoTSerial.hpp - Facade SoftwareSerial
 * ============================================================
 *
 * Communication via any serial module:
 *   HC-05, HC-06, HM-10, JDY-30, XBee, etc.
 *
 * Supported boards:
 *   Arduino Uno, Nano, Mega, ESP8266
 *
 * Usage:
 *   #include <InstantIoTSerial.hpp>
 *
 *   // RX_PIN = pin connected to module TX
 *   // TX_PIN = pin connected to module RX
 *   InstantIoTSerial instant(4, 14);
 *
 *   void setup() { instant.begin(); }
 *   void loop()  { instant.loop();  }
 *
 * ============================================================
 */

#if defined(ESP32)
    #error "InstantIoTSerial: use InstantIoTBluetoothESP32SPP or InstantIoTBluetoothBLE for ESP32"
#endif

#include "InstantIoTConfig.h"
#include "core/InstantIoTCore.hpp"
#include "transport/serial/InstantSoftwareSerial.hpp"

class InstantIoTSerial : public InstantIoT::InstantIoTCoreBase {
public:

    InstantIoTSerial(uint8_t rxPin, uint8_t txPin, long baud = INSTANT_SERIAL_BAUDRATE)
        : InstantIoT::InstantIoTCoreBase(_transportImpl)
        , _transportImpl(rxPin, txPin, baud)
    {}

private:
    InstantIoT::InstantSoftwareSerial _transportImpl;
};