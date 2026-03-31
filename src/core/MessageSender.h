#pragma once
/**
 * Interface pour envoyer des messages
 */

#include <stdint.h>
#include <stddef.h>

class IMessageSender {
public:
    virtual ~IMessageSender() = default;

    // Legacy — non utilisé en mode binaire
    virtual bool sendMessage(
        const char* widgetId,
        const char* widgetType,
        const char* event,
        const char* payload = nullptr
    ) = 0;

    // Binaire — envoi direct bytes
    virtual bool sendBinary(
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) = 0;

    virtual bool connected() = 0;
};