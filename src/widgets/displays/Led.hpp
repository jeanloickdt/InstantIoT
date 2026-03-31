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

    LedWidget& On()     { sendBinary(0x01); return *this; }
    LedWidget& Off()    { sendBinary(0x02); return *this; }
    LedWidget& toggle() { sendBinary(0x03); return *this; }

    LedWidget& setBrightness(uint8_t v) {
        sendBinary(EV_SETBRIGHTNESS, &v, 1);
        return *this;
    }

    LedWidget& setColor(uint8_t r, uint8_t g, uint8_t b) {
        uint8_t buf[3] = {r, g, b};
        sendBinary(EV_SETCOLOR, buf, 3);
        return *this;
    }

    LedWidget& setColor(uint32_t rgb) {
        return setColor((rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
    }
};

} // namespace InstantIoT