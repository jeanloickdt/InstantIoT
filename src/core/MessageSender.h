#pragma once
/**
 * Interface pour envoyer des messages
 */

class IMessageSender {
public:
    virtual ~IMessageSender() = default;
    virtual bool sendMessage(const char* widgetId, const char* widgetType, const char* event, const char* payload = nullptr) = 0;
    virtual bool connected() = 0;
};
