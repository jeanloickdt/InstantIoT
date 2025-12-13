#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class VerticalLevelWidget : public DisplayWidget {
public:
    VerticalLevelWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "verticallevel"; }
    
    VerticalLevelWidget& setValue(float value) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"value\":%.2f}", value);
        sendMessage("setvalue", payload);
        return *this;
    }
    
    VerticalLevelWidget& setRange(float min, float max) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"min\":%.2f,\"max\":%.2f}", min, max);
        sendMessage("setrange", payload);
        return *this;
    }

    VerticalLevelWidget& update(float value, float min, float max) {
        char payload[64];
        snprintf(payload, sizeof(payload),
            "{\"value\":%.2f,\"min\":%.2f,\"max\":%.2f}", value, min, max);
        sendMessage("update", payload);
        return *this;
    }
};

} // namespace InstantIoT
