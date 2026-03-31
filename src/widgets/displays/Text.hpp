#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

class TextWidget : public DisplayWidget {
public:
    TextWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_TEXT; }

    TextWidget& setText(const char* text) {
        uint8_t buf[129];
        size_t n = writeString(buf, text);
        sendBinary(EV_SETTEXT, buf, n);
        return *this;
    }
};

} // namespace InstantIoT