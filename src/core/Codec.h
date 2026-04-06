#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * Codec.h — Structure DecodedMessage
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

namespace InstantIoT {

/**
 * Message décodé depuis une trame binaire iWidgets v1
 */
struct DecodedMessage {
    const char* dashboardId;
    const char* deviceId;
    const char* widgetId;
    const char* widgetType;  // non utilisé en binaire — conservé pour compatibilité Registry
    const char* event;       // non utilisé en binaire — conservé pour compatibilité Registry

    // Payload décodé (jusqu'à 8 paramètres clé/valeur)
    struct Param {
        const char* key;
        const char* value;
    };
    Param   params[8];
    uint8_t paramCount;

    // ── Helpers ───────────────────────────────────────────────

    const char* getParam(const char* key) const {
        for (uint8_t i = 0; i < paramCount; i++) {
            if (strcmp(params[i].key, key) == 0) return params[i].value;
        }
        return nullptr;
    }

    float getParamFloat(const char* key, float defaultValue = 0.0f) const {
        const char* val = getParam(key);
        return val ? atof(val) : defaultValue;
    }

    int getParamInt(const char* key, int defaultValue = 0) const {
        const char* val = getParam(key);
        return val ? atoi(val) : defaultValue;
    }

    bool getParamBool(const char* key, bool defaultValue = false) const {
        const char* val = getParam(key);
        if (!val) return defaultValue;
        return (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
    }
};

} // namespace InstantIoT