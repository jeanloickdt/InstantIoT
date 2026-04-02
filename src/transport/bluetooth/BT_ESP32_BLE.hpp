#pragma once
/**
 * ============================================================
 * 📡 BT_ESP32_BLE.hpp - Transport BLE ESP32 via NimBLE
 * ============================================================
 *
 * Uses NimBLE-Arduino library (lightweight BLE stack).
 * Install via Arduino Library Manager: "NimBLE-Arduino"
 *
 * Board : ESP32 family (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
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

#if defined(ESP32)

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "../../core/Transport.h"
#include "../../InstantIoTConfig.h"

// ── Nordic UART Service (NUS) ─────────────────────────────────
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // App → Device
#define NUS_TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Device → App

#ifndef INSTANT_BLE_MTU
    #define INSTANT_BLE_MTU 512
#endif

namespace InstantIoT {

class BT_ESP32_BLE : public ITransport {
public:

    BT_ESP32_BLE(const char* deviceName)
        : _deviceName(deviceName)
        , _txChar(nullptr)
        , _rxHead(0)
        , _rxTail(0)
    {}

    // ── Lifecycle ─────────────────────────────────────────────

    bool begin() override {
        IIOT_LOG("[BLE-ESP32] Starting...");
        IIOT_LOG_VAL("[BLE-ESP32] Device name: ", _deviceName);

        NimBLEDevice::init(_deviceName);
        NimBLEDevice::setMTU(INSTANT_BLE_MTU);

        // Server
        NimBLEServer* server = NimBLEDevice::createServer();
        server->setCallbacks(new ServerCallbacks(this));

        // NUS Service
        NimBLEService* service = server->createService(NUS_SERVICE_UUID);

        // TX Characteristic — Device → App (notify)
        _txChar = service->createCharacteristic(
            NUS_TX_CHAR_UUID,
            NIMBLE_PROPERTY::NOTIFY
        );

        // RX Characteristic — App → Device (write)
        NimBLECharacteristic* rxChar = service->createCharacteristic(
            NUS_RX_CHAR_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
        );
        rxChar->setCallbacks(new CharCallbacks(this));

        // Advertising
        NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
        advertising->addServiceUUID(NUS_SERVICE_UUID);
        advertising->start();

        IIOT_LOG("[BLE-ESP32] Ready — advertising started");
        return true;
    }

    void poll() override {
        if (!isClientConnected()) {
            if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
                NimBLEDevice::getAdvertising()->start();
            }
        }
    }

    // ── Status ────────────────────────────────────────────────

    bool connected() override {
        return isClientConnected();
    }

    int available() override {
        if (_rxHead == _rxTail) return 0;
        return (_rxTail - _rxHead + RX_BUF_SIZE) % RX_BUF_SIZE;
    }

    // ── Read ──────────────────────────────────────────────────

    int read(uint8_t* buf, size_t len) override {
        size_t count = 0;
        while (count < len && available() > 0) {
            buf[count++] = _rxBuffer[_rxHead];
            _rxHead = (_rxHead + 1) % RX_BUF_SIZE;
        }
        return count > 0 ? (int)count : -1;
    }

    // ── Write ─────────────────────────────────────────────────

    size_t write(const uint8_t* buf, size_t len) override {
        if (!_txChar) return 0;
        _txChar->setValue(buf, len);
        _txChar->notify();
        return len;
    }

private:
    static const size_t RX_BUF_SIZE = 512;

    const char*           _deviceName;
    NimBLECharacteristic* _txChar;
    uint8_t               _rxBuffer[RX_BUF_SIZE];
    size_t                _rxHead;
    size_t                _rxTail;

    bool isClientConnected() {
        return NimBLEDevice::getServer() &&
               NimBLEDevice::getServer()->getConnectedCount() > 0;
    }

    void pushRxBytes(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            size_t next = (_rxTail + 1) % RX_BUF_SIZE;
            if (next != _rxHead) {
                _rxBuffer[_rxTail] = data[i];
                _rxTail = next;
            }
        }
    }

    // ── Server callbacks ──────────────────────────────────────
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BT_ESP32_BLE* t) : _t(t) {}

        void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
            IIOT_LOG("[BLE-ESP32] Client connected");
        }

        void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
            IIOT_LOG("[BLE-ESP32] Client disconnected");
        }

    private:
        BT_ESP32_BLE* _t;
    };

    // ── Characteristic callbacks ──────────────────────────────
    class CharCallbacks : public NimBLECharacteristicCallbacks {
    public:
        CharCallbacks(BT_ESP32_BLE* t) : _t(t) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            const uint8_t* data = pChar->getValue().data();
            size_t len = pChar->getValue().size();
            if (len > 0) _t->pushRxBytes(data, len);
        }

    private:
        BT_ESP32_BLE* _t;
    };
};

} // namespace InstantIoT

#endif // ESP32