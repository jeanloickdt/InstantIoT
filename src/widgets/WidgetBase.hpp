#pragma once
#include <Arduino.h>
#include "../InstantIoTConfig.h"
#include "../core/MessageSender.h"

namespace InstantIoT {

class WidgetBase {
protected:
    char _id[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    IMessageSender& _sender;

public:
    WidgetBase(const char* id, IMessageSender& sender) : _sender(sender) {
        strncpy(_id, id, sizeof(_id) - 1);
        _id[sizeof(_id) - 1] = '\0';
    }

    virtual ~WidgetBase() = default;
    const char* getId() const { return _id; }
    virtual uint8_t getTypeCode() const = 0;

protected:
    // Envoi binaire direct — utilisé par les widgets
    bool sendBinary(
        uint8_t eventCode,
        const uint8_t* payload = nullptr,
        size_t payloadLen = 0
    ) {
        return _sender.sendBinary(_id, getTypeCode(), eventCode, payload, payloadLen);
    }
};

class DisplayWidget : public WidgetBase {
public:
    using WidgetBase::WidgetBase;
};

} // namespace InstantIoT