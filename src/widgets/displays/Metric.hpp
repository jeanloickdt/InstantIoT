#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

class MetricWidget : public DisplayWidget {
public:
    MetricWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_METRIC; }

    MetricWidget& setValue(float value) {
        uint8_t buf[4]; writeFloatLE(buf, value);
        sendBinary(EV_SETVALUE, buf, 4);
        return *this;
    }

    MetricWidget& setSecondaryValue(const char* val, const char* label) {
        uint8_t buf[80]; size_t b = 0;
        b += writeString(buf+b, val);
        b += writeString(buf+b, label);
        sendBinary(EV_SETSECONDARY, buf, b);
        return *this;
    }
};

} // namespace InstantIoT