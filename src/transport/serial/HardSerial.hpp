#pragma once
/**
 * ============================================================
 * 📡 HardSerial.hpp - Transport over a hardware UART
 * ============================================================
 *
 * The same module as SoftSerial — HC-05, HC-06, HM-10, JDY-30 — on a
 * UART the chip already has, instead of two pins bit-banged in
 * software. This is how a board WITHOUT SoftwareSerial gets a serial
 * module: the Uno R4 WiFi, the MKR family, the Nano 33 IoT, the ESP32.
 * It is also the better choice on a Mega, which has three spare UARTs.
 *
 * Supported boards:
 *   every one that exposes a `HardwareSerial` beyond the USB one —
 *   `Serial1` on the R4, the MKRs, the Nano 33 IoT, the ESP32 family;
 *   `Serial1`…`Serial3` on the Mega. (The ESP8266's `Serial1` is
 *   transmit-only: keep SoftSerial there.)
 *
 * Wiring (Serial1 on an Uno R4 WiFi):
 *   Module RX → D1 / TX1 (voltage divider: the R4 drives 5 V)
 *   Module TX → D0 / RX1
 *   Module VCC → 3.3 V or 5 V (check module specs)
 *   Module GND → GND
 *
 * A hardware UART is not limited to 57600 baud the way SoftwareSerial
 * is; the default stays 9600 because that is what the modules ship at.
 *
 * ============================================================
 */

#include <Arduino.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

#ifndef INSTANT_SERIAL_BAUDRATE
    #define INSTANT_SERIAL_BAUDRATE 9600
#endif

#ifndef INSTANT_SERIAL_TIMEOUT_MS
    #define INSTANT_SERIAL_TIMEOUT_MS 5000
#endif

namespace iiot {

class HardSerial : public ITransport {
public:

    HardSerial(HardwareSerial& port, long baudrate = INSTANT_SERIAL_BAUDRATE)
        : _serial(port)
        , _baudrate(baudrate)
        , _connected(false)
        , _lastRxMs(0)
    {}

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        IIOT_LOG("[InstantSerial] Starting on hardware UART...");
        IIOT_LOG_VAL("[InstantSerial] Baudrate: ", _baudrate);
        _serial.begin(_baudrate);
        _connected = false;
        _lastRxMs  = 0;
        IIOT_LOG("[InstantSerial] Ready — waiting for connection");
        return true;
    }

    // Same presence rule as SoftSerial: a module has no "connected"
    // signal, so a byte received means someone is there, and silence
    // longer than the timeout means they left.
    void poll() override {
        if (_serial.available() > 0) {
            if (!_connected) {
                _connected = true;
                IIOT_LOG("[InstantSerial] Device connected");
            }
            _lastRxMs = millis();
        }
        if (_connected && (millis() - _lastRxMs > INSTANT_SERIAL_TIMEOUT_MS)) {
            _connected = false;
            IIOT_LOG("[InstantSerial] Device disconnected (timeout)");
        }
    }

    // ============================================================
    // 📡 STATUS
    // ============================================================

    // Always true, as with SoftSerial: the wire is always there, and the
    // facade must keep writing so the module has something to forward
    // the moment a phone pairs.
    bool connected() override { return true; }
    int  available() override { return _serial.available(); }

    // ============================================================
    // 📥 READ / 📤 WRITE
    // ============================================================

    int read(uint8_t* buf, size_t len) override {
        size_t count = 0;
        while (count < len && _serial.available() > 0) {
            buf[count++] = (uint8_t)_serial.read();
        }
        return count > 0 ? (int)count : -1;
    }

    size_t write(const uint8_t* buf, size_t len) override {
        return _serial.write(buf, len);
    }

private:
    HardwareSerial& _serial;
    long            _baudrate;
    bool            _connected;
    uint32_t        _lastRxMs;
};

} // namespace iiot
