#pragma once
/**
 * ============================================================
 * 🚚 Transport.h - Abstract transport interface
 * ============================================================
 *
 * Common interface for all transports:
 * - WiFi SoftAP (ESP32, ESP8266, UNO R4)
 * - WiFi Station
 * - Bluetooth BLE
 * - Serial
 * 
 * ============================================================
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * Abstract transport interface
 */
struct ITransport {
    
    virtual ~ITransport() = default;
    
    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================
    
    /**
     * Initializes the transport
     * @return true on success
     */
    virtual bool begin() = 0;
    
    /**
     * Called in loop() to handle connections
     */
    virtual void poll() = 0;
    
    // ============================================================
    // 📡 STATUS
    // ============================================================
    
    /**
     * @return true if a client is connected
     */
    virtual bool connected() = 0;
    
    /**
     * @return Number of bytes available to read
     */
    virtual int available() = 0;
    
    // ============================================================
    // 📥 READ
    // ============================================================

    /**
     * Reads data from the transport
     *
     * @param buf Destination buffer
     * @param len Max size to read
     * @return Number of bytes read, -1 on error
     */
    virtual int read(uint8_t* buf, size_t len) = 0;
    
    // ============================================================
    // 📤 WRITE
    // ============================================================

    /**
     * Writes data to the transport
     *
     * @param buf Source buffer
     * @param len Size to write
     * @return Number of bytes written
     */
    virtual size_t write(const uint8_t* buf, size_t len) = 0;
    
    // ============================================================
    // 🔧 HELPERS
    // ============================================================
    
    size_t print(const char* str) {
        if (!str) return 0;
        return write(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }
    
    size_t println(const char* str) {
        size_t n = print(str);
        n += write(reinterpret_cast<const uint8_t*>("\n"), 1);
        return n;
    }
};
