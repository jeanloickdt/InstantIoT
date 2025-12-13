#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class BarChartWidget : public DisplayWidget {
public:
    BarChartWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "barchart"; }
    
    // Définir une barre par index
    BarChartWidget& setBar(int index, float value) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"index\":%d,\"value\":%.2f}", index, value);
        sendMessage("setbar", payload);
        return *this;
    }
    
    // Définir une barre par label/id
    BarChartWidget& setBar(const char* barId, float value) {
        char payload[64];
        snprintf(payload, sizeof(payload), "{\"barId\":\"%s\",\"value\":%.2f}", barId, value);
        sendMessage("setbar", payload);
        return *this;
    }
    
    // Définir toutes les barres d'un coup (max 8)
    BarChartWidget& setValues(float v0, float v1 = 0, float v2 = 0, float v3 = 0, 
                               float v4 = 0, float v5 = 0, float v6 = 0, float v7 = 0) {
        char payload[128];
        snprintf(payload, sizeof(payload), 
            "{\"values\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]}", 
            v0, v1, v2, v3, v4, v5, v6, v7);
        sendMessage("setvalues", payload);
        return *this;
    }
    
    // Clear
    BarChartWidget& clear() {
        sendMessage("clear");
        return *this;
    }
};

} // namespace InstantIoT
