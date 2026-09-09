#pragma once

/*************************************************************
 * ⚡ InstantIoTDeviceConfig.hpp — what the board calls itself
 *
 * ## The board does not name itself, and that is the point
 *
 * Its identity is its TOKEN, presented once at the handshake. From that
 * moment the relay knows which board is on the other end of the socket,
 * and every SIGNAL frame says so by writing `DEV_COUNT = 0` —
 * `BinaryCodec::encodeSignal`, "the relay knows the board".
 *
 * The heartbeat used to disagree. It went through the older encoder, which
 * carries a device field, and so it shipped an identifier on every beat.
 *
 * That identifier was generated from the chip: the eFuse MAC on an ESP32,
 * the chip id on an ESP8266 — and, on every board without a serial number
 * of its own, the literal `device_DEADBEEF`. Identical on two Nano 33 IoT
 * sitting on the same desk. Which mattered not at all, because **nothing
 * reads it**:
 *
 *   - the relay skips the device section on every read path
 *     (`FrameParser.extractControlId`, `extractType`, …) and filters
 *     heartbeats out before the app ever sees them;
 *   - in Direct mode the app builds its key from the device field of
 *     SIGNAL frames, and those carry none — there is one board at the far
 *     end of the wire, and no ambiguity to resolve.
 *
 * So the fix was not a better invented name. It was to stop inventing one:
 * the default is empty, the encoder already writes `DEV_COUNT = 0` for an
 * empty name, and the beat is sixteen bytes shorter — every five seconds,
 * through TLS, forever.
 *
 * ## `setDeviceId()` still does exactly what it says
 *
 * A sketch that names its board gets that name in its heartbeats, as
 * before. What disappeared is the name nobody chose.
 *
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>
#include "../InstantIoTConfig.h"

namespace iiot {

/**
 * @brief InstantIoT device configuration
 *
 * Contains the identifiers needed for communication
 * with the mobile application.
 */
class DeviceConfig {
private:
    char _dashboardId[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    char _deviceId[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    char _deviceName[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    
public:
    DeviceConfig() {
        strcpy(_dashboardId, "default");
        // Empty on purpose — see the header. An empty name makes the encoder
        // write DEV_COUNT = 0, which is what every SIGNAL frame already does.
        _deviceId[0] = '\0';
        strcpy(_deviceName, "InstantIoT Device");
    }
    
    // ════════════════════════════════════════════════════════
    // 🔧 SETTERS
    // ════════════════════════════════════════════════════════
    
    /**
     * @brief Sets the dashboard ID
     */
    DeviceConfig& setDashboardId(const char* id) {
        if (id) {
            strncpy(_dashboardId, id, sizeof(_dashboardId) - 1);
            _dashboardId[sizeof(_dashboardId) - 1] = '\0';
        }
        return *this;
    }
    
    /**
     * Names the board — optional, and empty by default.
     *
     * The name rides in the heartbeat's device field. Nothing on the server
     * side reads it today; it is there for a fleet that wants its own label
     * in a capture. The board's real identity is its token.
     */
    DeviceConfig& setDeviceId(const char* id) {
        if (id) {
            strncpy(_deviceId, id, sizeof(_deviceId) - 1);
            _deviceId[sizeof(_deviceId) - 1] = '\0';
        }
        return *this;
    }
    
    /**
     * @brief Sets the device name (displayed in the app)
     */
    DeviceConfig& setDeviceName(const char* name) {
        if (name) {
            strncpy(_deviceName, name, sizeof(_deviceName) - 1);
            _deviceName[sizeof(_deviceName) - 1] = '\0';
        }
        return *this;
    }
    
    // ════════════════════════════════════════════════════════
    // 🔍 GETTERS
    // ════════════════════════════════════════════════════════
    
    const char* getDashboardId() const { return _dashboardId; }
    const char* getDeviceId() const { return _deviceId; }
    const char* getDeviceName() const { return _deviceName; }
    
};

} // namespace iiot
