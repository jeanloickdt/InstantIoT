#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

class LedWidget : public DisplayWidget {
public:
    LedWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_LED; }

    // ── On / Off / Toggle ─────────────────────────────────────
    LedWidget& On()     { sendBinary(0x01); return *this; }
    LedWidget& Off()    { sendBinary(0x02); return *this; }
    LedWidget& toggle() { sendBinary(0x03); return *this; }

    // Alias lowercase pour compatibilité exemples
    LedWidget& on()     { return On(); }
    LedWidget& off()    { return Off(); }
    LedWidget& turnOn() { return On(); }
    LedWidget& turnOff(){ return Off(); }

    // ── Brightness ────────────────────────────────────────────
    LedWidget& setBrightness(uint8_t v) {
        sendBinary(EV_SETBRIGHTNESS, &v, 1);
        return *this;
    }

    // Alias — setIntensity accepte float 0.0..1.0 ou 0..100
    LedWidget& setIntensity(float value) {
        uint8_t v = (uint8_t)(value <= 1.0f ? value * 100 : value);
        return setBrightness(v);
    }

    // ── Color ─────────────────────────────────────────────────
    LedWidget& setColor(uint8_t r, uint8_t g, uint8_t b) {
        uint8_t buf[3] = {r, g, b};
        sendBinary(EV_SETCOLOR, buf, 3);
        return *this;
    }

    LedWidget& setColor(uint32_t rgb) {
        return setColor((rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
    }

    // Alias setColors — dans le proto v1 on envoie juste la couleur led
    LedWidget& setColors(uint32_t led, uint32_t /*halo*/, uint32_t /*rays*/) {
        return setColor(led);
    }

    // ── setState ──────────────────────────────────────────────
    LedWidget& setState(bool on, float intensity = 1.0f) {
        if (on) { setIntensity(intensity); On(); }
        else Off();
        return *this;
    }

    LedWidget& setBrightness(float value) {
        return setIntensity(value);
    }

    // ── showHalo / showRays ───────────────────────────────────
    // Non supporté en protocole binaire v1 — no-op pour compatibilité
    LedWidget& showHalo(bool) { return *this; }
    LedWidget& showRays(bool) { return *this; }
};

} // namespace InstantIoT