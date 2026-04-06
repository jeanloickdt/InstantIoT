#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v1.1.0
 * 
 * InstantIoTDeviceConfig.hpp - Configuration et identité du device
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>
#include "../InstantIoTConfig.h"

namespace InstantIoT {

/**
 * @brief Configuration du device InstantIoT
 * 
 * Contient les identifiants nécessaires pour la communication
 * avec l'application mobile.
 */
class DeviceConfig {
private:
    char _dashboardId[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    char _deviceId[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    char _deviceName[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    
public:
    DeviceConfig() {
        // Valeurs par défaut
        strcpy(_dashboardId, "default");
        generateDeviceId();
        strcpy(_deviceName, "InstantIoT Device");
    }
    
    // ════════════════════════════════════════════════════════
    // 🔧 SETTERS
    // ════════════════════════════════════════════════════════
    
    /**
     * @brief Définit l'ID du dashboard
     */
    DeviceConfig& setDashboardId(const char* id) {
        if (id) {
            strncpy(_dashboardId, id, sizeof(_dashboardId) - 1);
            _dashboardId[sizeof(_dashboardId) - 1] = '\0';
        }
        return *this;
    }
    
    /**
     * @brief Définit l'ID du device
     */
    DeviceConfig& setDeviceId(const char* id) {
        if (id) {
            strncpy(_deviceId, id, sizeof(_deviceId) - 1);
            _deviceId[sizeof(_deviceId) - 1] = '\0';
        }
        return *this;
    }
    
    /**
     * @brief Définit le nom du device (affiché dans l'app)
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
    
private:
    /**
     * @brief Génère un ID device unique basé sur le MAC/chip ID
     */
    void generateDeviceId() {
        #if defined(INSTANTIOT_PLATFORM_ESP32)
            uint64_t chipId = ESP.getEfuseMac();
            snprintf(_deviceId, sizeof(_deviceId), "esp32_%04X%08X",
                (uint16_t)(chipId >> 32),
                (uint32_t)chipId
            );
        #elif defined(INSTANTIOT_PLATFORM_ESP8266)
            snprintf(_deviceId, sizeof(_deviceId), "esp8266_%08X", ESP.getChipId());

        #elif defined(INSTANTIOT_PLATFORM_R4)
            // R4 has no unique hardware ID — use fixed seed
            // Override with: instant.config().setDeviceId("my_device")
            snprintf(_deviceId, sizeof(_deviceId), "r4_%08lX", (unsigned long)0xDEADBEEF);
        #else
            // Unknown platform — use fixed seed
            snprintf(_deviceId, sizeof(_deviceId), "device_%08lX", (unsigned long)0xDEADBEEF);
        #endif
    }
};

} // namespace InstantIoT
