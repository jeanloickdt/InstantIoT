#pragma once
/**
 * ============================================================
 * 🚚 Transport.h - Interface transport abstraite
 * ============================================================
 * 
 * Interface commune pour tous les transports :
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
 * Interface de transport abstraite
 */
struct ITransport {
    
    virtual ~ITransport() = default;
    
    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================
    
    /**
     * Initialise le transport
     * @return true si succès
     */
    virtual bool begin() = 0;
    
    /**
     * Appelé dans loop() pour gérer les connexions
     */
    virtual void poll() = 0;
    
    // ============================================================
    // 📡 STATUS
    // ============================================================
    
    /**
     * @return true si un client est connecté
     */
    virtual bool connected() = 0;
    
    /**
     * @return Nombre de bytes disponibles en lecture
     */
    virtual int available() = 0;
    
    // ============================================================
    // 📥 LECTURE
    // ============================================================
    
    /**
     * Lit des données depuis le transport
     * 
     * @param buf Buffer destination
     * @param len Taille max à lire
     * @return Nombre de bytes lus, -1 si erreur
     */
    virtual int read(uint8_t* buf, size_t len) = 0;
    
    // ============================================================
    // 📤 ÉCRITURE
    // ============================================================
    
    /**
     * Écrit des données vers le transport
     * 
     * @param buf Buffer source
     * @param len Taille à écrire
     * @return Nombre de bytes écrits
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
