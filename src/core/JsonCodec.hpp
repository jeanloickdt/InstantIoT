#pragma once
/**
 * ============================================================
 * 🔐 JsonCodec.hpp - Codec JSON léger
 * ============================================================
 */

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include "Codec.h"
#include "../InstantIoTConfig.h"

namespace InstantIoT {

class JsonCodec {
private:
    char _dashboardId[32];
    char _deviceId[32];
    char _widgetId[32];
    char _widgetType[32];
    char _event[32];
    char _paramKeys[8][16];
    char _paramValues[8][32];
    
    bool extractString(const char* json, const char* key, char* output, size_t outputSize) {
        char searchPattern[48];
        snprintf(searchPattern, sizeof(searchPattern), "\"%s\":\"", key);
        
        const char* start = strstr(json, searchPattern);
        if (!start) return false;
        
        start += strlen(searchPattern);
        const char* end = strchr(start, '"');
        if (!end) return false;
        
        size_t len = end - start;
        if (len >= outputSize) len = outputSize - 1;
        
        strncpy(output, start, len);
        output[len] = '\0';
        return true;
    }
    
    bool extractPayload(const char* json, DecodedMessage& msg) {
        const char* payloadStart = strstr(json, "\"payload\":{");
        if (!payloadStart) {
            payloadStart = strstr(json, "\"payload\": {");
            if (!payloadStart) return true;
            payloadStart += 11;
        } else {
            payloadStart += 10;
        }

        msg.paramCount = 0;
        const char* current = payloadStart;

        while (*current && *current != '}' && msg.paramCount < 8) {
            const char* keyStart = strchr(current, '"');
            if (!keyStart || keyStart > strchr(current, '}')) break;
            keyStart++;

            const char* keyEnd = strchr(keyStart, '"');
            if (!keyEnd) break;

            size_t keyLen = keyEnd - keyStart;
            if (keyLen >= 16) keyLen = 15;
            strncpy(_paramKeys[msg.paramCount], keyStart, keyLen);
            _paramKeys[msg.paramCount][keyLen] = '\0';

            const char* valueStart = strchr(keyEnd, ':');
            if (!valueStart) break;
            valueStart++;
            while (*valueStart == ' ') valueStart++;

            // Initialiser le buffer à vide
            _paramValues[msg.paramCount][0] = '\0';

            if (*valueStart == '"') {
                // Valeur string
                valueStart++;
                const char* valueEnd = strchr(valueStart, '"');
                if (!valueEnd) break;

                size_t valLen = valueEnd - valueStart;
                if (valLen >= 32) valLen = 31;
                if (valLen > 0) {
                    memcpy(_paramValues[msg.paramCount], valueStart, valLen);
                    _paramValues[msg.paramCount][valLen] = '\0';
                }
                current = valueEnd + 1;
            } else {
                // Valeur non-string (number, bool, etc.)
                const char* valueEnd = valueStart;
                while (*valueEnd && *valueEnd != ',' && *valueEnd != '}') valueEnd++;

                size_t valLen = valueEnd - valueStart;
                if (valLen >= 32) valLen = 31;

                if (valLen > 0) {
                    // Copier la valeur
                    memcpy(_paramValues[msg.paramCount], valueStart, valLen);
                    _paramValues[msg.paramCount][valLen] = '\0';

                    // Trim trailing whitespace inline (sans strlen)
                    char* p = _paramValues[msg.paramCount];
                    char* lastNonSpace = p;
                    while (*p) {
                        if (*p != ' ' && *p != '\n' && *p != '\r') {
                            lastNonSpace = p;
                        }
                        p++;
                    }
                    if (lastNonSpace < p - 1) {
                        *(lastNonSpace + 1) = '\0';
                    }
                }
                current = valueEnd;
            }

            msg.params[msg.paramCount].key = _paramKeys[msg.paramCount];
            msg.params[msg.paramCount].value = _paramValues[msg.paramCount];
            msg.paramCount++;

            const char* comma = strchr(current, ',');
            const char* brace = strchr(current, '}');
            if (!comma || (brace && brace < comma)) break;
            current = comma + 1;
        }
        return true;
    }

public:
    JsonCodec() {
        _dashboardId[0] = '\0';
        _deviceId[0] = '\0';
        _widgetId[0] = '\0';
        _widgetType[0] = '\0';
        _event[0] = '\0';
        for (int i = 0; i < 8; i++) {
            _paramKeys[i][0] = '\0';
            _paramValues[i][0] = '\0';
        }
    }

    size_t encode(
        char* buffer,
        size_t bufferSize,
        const char* dashboardId,
        const char* deviceId,
        const char* widgetId,
        const char* widgetType,
        const char* event,
        const char* payload = nullptr
    ) {
        int written;

        if (payload && payload[0] != '\0') {
            written = snprintf(buffer, bufferSize,
                "{\"dashboardId\":\"%s\",\"deviceId\":\"%s\",\"widgetId\":\"%s\",\"type\":\"%s\",\"dir\":\"dev->ui\",\"event\":\"%s\",\"payload\":%s}",
                dashboardId ? dashboardId : "",
                deviceId ? deviceId : "",
                widgetId ? widgetId : "",
                widgetType ? widgetType : "",
                event ? event : "",
                payload
            );
        } else {
            written = snprintf(buffer, bufferSize,
                "{\"dashboardId\":\"%s\",\"deviceId\":\"%s\",\"widgetId\":\"%s\",\"type\":\"%s\",\"dir\":\"dev->ui\",\"event\":\"%s\"}",
                dashboardId ? dashboardId : "",
                deviceId ? deviceId : "",
                widgetId ? widgetId : "",
                widgetType ? widgetType : "",
                event ? event : ""
            );
        }

        if (written < 0 || (size_t)written >= bufferSize) return 0;
        return (size_t)written;
    }

    bool decode(const char* buffer, size_t length, DecodedMessage& outMessage) {
        if (!buffer || length == 0) return false;

        memset(&outMessage, 0, sizeof(DecodedMessage));

        if (!extractString(buffer, "widgetId", _widgetId, sizeof(_widgetId))) {
            IIOT_LOG("[JsonCodec] Missing widgetId");
            return false;
        }

        if (!extractString(buffer, "type", _widgetType, sizeof(_widgetType))) {
            if (!extractString(buffer, "widgetType", _widgetType, sizeof(_widgetType))) {
                IIOT_LOG("[JsonCodec] Missing type");
                return false;
            }
        }

        if (!extractString(buffer, "event", _event, sizeof(_event))) {
            IIOT_LOG("[JsonCodec] Missing event");
            return false;
        }

        extractString(buffer, "dashboardId", _dashboardId, sizeof(_dashboardId));
        extractString(buffer, "deviceId", _deviceId, sizeof(_deviceId));

        outMessage.dashboardId = _dashboardId;
        outMessage.deviceId = _deviceId;
        outMessage.widgetId = _widgetId;
        outMessage.widgetType = _widgetType;
        outMessage.event = _event;

        extractPayload(buffer, outMessage);

        IIOT_LOG_3("[JsonCodec] Decoded: type=", _widgetType, " id=", _widgetId, " event=", _event);

        return true;
    }

    const char* contentType() const { return "application/json"; }
};

} // namespace InstantIoT