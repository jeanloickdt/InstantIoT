#pragma once
/**
 * ============================================================
 * 📤 MessageSender.h — Interface d'envoi binaire iWidgets v1
 * ============================================================
 */

#include <stdint.h>
#include <stddef.h>

class IMessageSender {
public:
    virtual ~IMessageSender() = default;

    /**
     * Envoie une trame binaire iWidgets v1
     *
     * @param widgetId   Protocol ID du widget (ex: "led1")
     * @param typeCode   Code TYPE (ex: TYPE_LED = 0x0C)
     * @param eventCode  Code EVENT (ex: EV_SETCOLOR = 0x05)
     * @param payloadBytes Bytes du payload (nullptr si aucun)
     * @param payloadLen   Taille du payload
     * @return true si envoyé
     */
    virtual bool sendBinary(
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) = 0;

    /**
     * @return true si un client est connecté
     */
    virtual bool connected() = 0;
};