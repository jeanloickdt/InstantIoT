#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

class VerticalLevelWidget : public DisplayWidget {
public:
    VerticalLevelWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_VLEVEL; }

    VerticalLevelWidget& setValue(float value) {
        uint8_t buf[4]; writeFloatLE(buf, value);
        sendBinary(EV_SETVALUE, buf, 4);
        return *this;
    }

    VerticalLevelWidget& setRange(float min, float max) {
        uint8_t buf[8]; writeFloatLE(buf, min); writeFloatLE(buf+4, max);
        sendBinary(EV_SETRANGE, buf, 8);
        return *this;
    }

    VerticalLevelWidget& update(float value, float min, float max) {
        uint8_t buf[12];
        writeFloatLE(buf, value); writeFloatLE(buf+4, min); writeFloatLE(buf+8, max);
        sendBinary(EV_UPDATE, buf, 12);
        return *this;
    }
};

} // namespace InstantIoT