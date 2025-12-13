#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class GaugeWidget : public DisplayWidget {
public:
    GaugeWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "gauge"; }
    
    GaugeWidget& setValue(float value) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"value\":%.2f}", value);
        sendMessage("setvalue", payload);
        return *this;
    }
    
    GaugeWidget& setRange(float min, float max) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"min\":%.2f,\"max\":%.2f}", min, max);
        sendMessage("setrange", payload);
        return *this;
    }
    
    GaugeWidget& setUnit(const char* unit) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"unit\":\"%s\"}", unit);
        sendMessage("setunit", payload);
        return *this;
    }

    /**
     * @brief Update value and range in one message (atomic)
     * @param value Current value
     * @param min Minimum value
     * @param max Maximum value
     */
    GaugeWidget& update(float value, float min, float max) {
        char payload[64];
        snprintf(payload, sizeof(payload),
            "{\"value\":%.2f,\"min\":%.2f,\"max\":%.2f}",
            value, min, max);
        sendMessage("update", payload);
        return *this;
    }
};

} // namespace InstantIoT
