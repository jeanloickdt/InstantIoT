#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class TextWidget : public DisplayWidget {
public:
    TextWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "text"; }
    
    TextWidget& setText(const char* text) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"text\":\"%s\"}", text);
        sendMessage("settext", payload);
        return *this;
    }


    TextWidget& setColor(uint32_t rgb) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"color\":\"#%06lX\"}", (unsigned long)(rgb & 0xFFFFFF));
        sendMessage("setcolor", payload);
        return *this;
    }
};

} // namespace InstantIoT
