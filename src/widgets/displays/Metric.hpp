#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class MetricWidget : public DisplayWidget {
public:
    MetricWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "metric"; }
    
    MetricWidget& setValue(float value) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"value\":%.2f}", value);
        sendMessage("setvalue", payload);
        return *this;
    }
    
    MetricWidget& setUnit(const char* unit) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"unit\":\"%s\"}", unit);
        sendMessage("setunit", payload);
        return *this;
    }
};

} // namespace InstantIoT
