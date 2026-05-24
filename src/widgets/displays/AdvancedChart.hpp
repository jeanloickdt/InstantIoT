#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

class AdvancedChartWidget : public DisplayWidget {
    int _pointIndex = 0;

public:
    AdvancedChartWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender), _pointIndex(0) {}

    uint8_t getTypeCode() const override { return TYPE_ADVANCEDCHART; }

    AdvancedChartWidget& addPoint(const char* seriesId, float y) {
        uint8_t buf[64]; size_t b = 0;
        b += writeString(buf+b, seriesId);
        writeFloatLE(buf+b, y); b += 4;
        sendBinary(EV_ADDPOINT, buf, b);
        _pointIndex++;
        return *this;
    }

    AdvancedChartWidget& addTimedPoint(const char* seriesId, float x, float y) {
        uint8_t buf[64]; size_t b = 0;
        b += writeString(buf+b, seriesId);
        writeFloatLE(buf+b, x); b += 4;
        writeFloatLE(buf+b, y); b += 4;
        sendBinary(EV_ADDTIMEDPOINT, buf, b);
        return *this;
    }

    AdvancedChartWidget& addPoint(float y) { return addPoint("default", y); }

    /**
     * Push a complete data series in one frame. Useful for boot-time
     * restoration or batch updates from a local buffer.
     *
     * Payload format (EV_SETSERIESDATA = 0x05):
     *   [seriesId_len:u8 | seriesId_bytes | count:u16_LE | points×float_LE]
     */
    AdvancedChartWidget& setSeriesData(const char* seriesId, const float* points, uint16_t count) {
        // Bornes raisonnables : ne pas dépasser le buffer TX
        if (count > 200) count = 200; // 200 * 4 + ~32 = ~832 bytes max payload
        uint8_t buf[1024];
        size_t p = 0;
        size_t sidLen = seriesId ? strlen(seriesId) : 0;
        if (sidLen > 255) sidLen = 255;
        buf[p++] = (uint8_t)sidLen;
        if (sidLen) { memcpy(buf + p, seriesId, sidLen); p += sidLen; }
        buf[p++] = (uint8_t)(count & 0xFF);
        buf[p++] = (uint8_t)((count >> 8) & 0xFF);
        for (uint16_t i = 0; i < count; i++) {
            uint32_t bits;
            memcpy(&bits, &points[i], 4);
            buf[p++] = (uint8_t)(bits & 0xFF);
            buf[p++] = (uint8_t)((bits >> 8) & 0xFF);
            buf[p++] = (uint8_t)((bits >> 16) & 0xFF);
            buf[p++] = (uint8_t)((bits >> 24) & 0xFF);
        }
        sendBinary(EV_SETSERIESDATA, buf, p);
        return *this;
    }

    AdvancedChartWidget& clearSeries(const char* seriesId) {
        uint8_t buf[32];
        size_t n = writeString(buf, seriesId);
        sendBinary(EV_CLEARSERIES, buf, n);
        return *this;
    }

    AdvancedChartWidget& clear() {
        _pointIndex = 0;
        sendBinary(EV_CLEARALL);
        return *this;
    }

    AdvancedChartWidget& resetIndex() { _pointIndex = 0; return *this; }
};

} // namespace InstantIoT