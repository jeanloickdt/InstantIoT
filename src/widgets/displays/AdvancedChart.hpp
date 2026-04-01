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