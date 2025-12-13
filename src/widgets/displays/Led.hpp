#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../InstantIoTConfig.h"

namespace InstantIoT {

class LedWidget : public DisplayWidget {
public:
    LedWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}
    
    const char* getType() const override { return "led"; }
    
    LedWidget& On() { sendMessage("turnon"); return *this; }
    LedWidget& Off() { sendMessage("turnoff"); return *this; }
    LedWidget& toggle() { sendMessage("toggle"); return *this; }
    
    // Kotlin attend "setbrightness" avec "brightness"
    LedWidget& setIntensity(float value) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"brightness\":%.2f}", value);
        sendMessage("setbrightness", payload);
        return *this;
    }

    // Alias pour clarté
    LedWidget& setBrightness(float value) { return setIntensity(value); }

    // Kotlin attend "updateoncolors" avec "led" en hex string
    LedWidget& setColor(uint32_t rgb) {
        char payload[48];
        snprintf(payload, sizeof(payload), "{\"led\":\"#%06lX\"}", (unsigned long)rgb);
        sendMessage("updateoncolors", payload);
        return *this;
    }

    // Version avec couleurs séparées (led, halo, rays)
    LedWidget& setColors(uint32_t led, uint32_t halo, uint32_t rays) {
        char payload[96];
        snprintf(payload, sizeof(payload),
            "{\"led\":\"#%06lX\",\"halo\":\"#%06lX\",\"rays\":\"#%06lX\"}",
            (unsigned long)led, (unsigned long)halo, (unsigned long)rays);
        sendMessage("updateoncolors", payload);
        return *this;
    }

    // setState combiné - utilise turnon/turnoff + brightness
    LedWidget& setState(bool on, float intensity = 1.0f) {
        if (on) {
            setIntensity(intensity);
            On();
        } else {
            Off();
        }
        return *this;
    }

    // Afficher/masquer le halo
    LedWidget& showHalo(bool show) {
        char payload[24];
        snprintf(payload, sizeof(payload), "{\"show\":%s}", show ? "true" : "false");
        sendMessage("showhalo", payload);
        return *this;
    }

    // Afficher/masquer les rayons
    LedWidget& showRays(bool show) {
        char payload[24];
        snprintf(payload, sizeof(payload), "{\"show\":%s}", show ? "true" : "false");
        sendMessage("showrays", payload);
        return *this;
    }
};

} // namespace InstantIoT