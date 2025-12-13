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
    virtual const char* getType() const = 0;
    
protected:
    bool sendMessage(const char* event, const char* payload = nullptr) {
        return _sender.sendMessage(_id, getType(), event, payload);
    }
};

class DisplayWidget : public WidgetBase {
public:
    using WidgetBase::WidgetBase;
};

} // namespace InstantIoT
