#pragma once
/**
 * ============================================================
 * 📤 MessageSender.h — iWidgets v1 binary send interface
 * ============================================================
 */

#include <stdint.h>
#include <stddef.h>

class IMessageSender {
public:
    virtual ~IMessageSender() = default;

    /**
     * Sends an iWidgets v1 binary frame
     *
     * @param widgetId   Widget protocol ID (e.g. "led1")
     * @param typeCode   TYPE code (e.g. TYPE_LED = 0x0C)
     * @param eventCode  EVENT code (e.g. EV_SETCOLOR = 0x05)
     * @param payloadBytes Payload bytes (nullptr if none)
     * @param payloadLen   Payload size
     * @return true if sent
     */
    virtual bool sendBinary(
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) = 0;

    /**
     * @return true if a client is connected
     */
    virtual bool connected() = 0;
};