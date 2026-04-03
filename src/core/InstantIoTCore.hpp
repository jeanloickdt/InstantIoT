#pragma once
/**
 * ============================================================
 * 🔄 InstantIoTCore.hpp - Boucle principale — protocole binaire v1
 * ============================================================
 */

#include <Arduino.h>
#include "Transport.h"
#include "BinaryCodec.hpp"
#include "Registry.hpp"
#include "InstantIoTDeviceConfig.hpp"
#include "InstantIoTMessage.hpp"
#include "MessageSender.h"
#include "../InstantIoTConfig.h"
#include "../widgets/WidgetIncludes.hpp"

namespace InstantIoT {

class InstantIoTCoreBase : public IMessageSender {
public:

    InstantIoTCoreBase(ITransport& transport)
        : _transport(transport)
        , _rxPos(0)
        , _initialized(false)
    {}

    virtual ~InstantIoTCoreBase() {
        #ifdef INSTANTIOT_WIDGETS_LED
        for (uint8_t i = 0; i < _ledCount; i++) delete _leds[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_GAUGE
        for (uint8_t i = 0; i < _gaugeCount; i++) delete _gauges[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_METRIC
        for (uint8_t i = 0; i < _metricCount; i++) delete _metrics[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
        for (uint8_t i = 0; i < _hLevelCount; i++) delete _hLevels[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_VERTICALLEVEL
        for (uint8_t i = 0; i < _vLevelCount; i++) delete _vLevels[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_ADVANCEDCHART
        for (uint8_t i = 0; i < _chartCount; i++) delete _charts[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_BARCHART
        for (uint8_t i = 0; i < _barChartCount; i++) delete _barCharts[i];
        #endif
        #ifdef INSTANTIOT_WIDGETS_TEXT
        for (uint8_t i = 0; i < _textCount; i++) delete _texts[i];
        #endif
    }

    // ════════════════════════════════════════════════════════
    //  LIFECYCLE
    // ════════════════════════════════════════════════════════

    virtual bool begin() {
        IIOT_LOG("[InstantIoT] Starting...");
        if (!_transport.begin()) {
            IIOT_LOG("[InstantIoT] Transport FAILED");
            return false;
        }
        _initialized = true;
        IIOT_LOG("[InstantIoT] Ready");
        return true;
    }

    virtual void loop() {
        if (!_initialized) return;
        _transport.poll();
        readLoop();
    }

    bool connected() override {
        return _transport.connected();
    }

    bool sendBinary(
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) override {
        if (!_transport.connected()) return false;

        size_t len = _codec.encode(
            _txBuffer, sizeof(_txBuffer),
            _config.getDeviceId(),
            widgetId,
            typeCode,
            eventCode,
            payloadBytes,
            payloadLen
        );

        if (len == 0) return false;
        return _transport.write(_txBuffer, len) == len;
    }

    // ════════════════════════════════════════════════════════
    // 📊 WIDGET ACCESS
    // ════════════════════════════════════════════════════════

    #ifdef INSTANTIOT_WIDGETS_LED
    LedWidget& led(const char* id) {
        for (uint8_t i = 0; i < _ledCount; i++)
            if (strcmp(_leds[i]->getId(), id) == 0) return *_leds[i];
        if (_ledCount < INSTANTIOT_MAX_WIDGETS) {
            _leds[_ledCount] = new LedWidget(id, *this);
            return *_leds[_ledCount++];
        }
        static LedWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_GAUGE
    GaugeWidget& gauge(const char* id) {
        for (uint8_t i = 0; i < _gaugeCount; i++)
            if (strcmp(_gauges[i]->getId(), id) == 0) return *_gauges[i];
        if (_gaugeCount < INSTANTIOT_MAX_WIDGETS) {
            _gauges[_gaugeCount] = new GaugeWidget(id, *this);
            return *_gauges[_gaugeCount++];
        }
        static GaugeWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_METRIC
    MetricWidget& metric(const char* id) {
        for (uint8_t i = 0; i < _metricCount; i++)
            if (strcmp(_metrics[i]->getId(), id) == 0) return *_metrics[i];
        if (_metricCount < INSTANTIOT_MAX_WIDGETS) {
            _metrics[_metricCount] = new MetricWidget(id, *this);
            return *_metrics[_metricCount++];
        }
        static MetricWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
    HorizontalLevelWidget& hLevel(const char* id) {
        for (uint8_t i = 0; i < _hLevelCount; i++)
            if (strcmp(_hLevels[i]->getId(), id) == 0) return *_hLevels[i];
        if (_hLevelCount < INSTANTIOT_MAX_WIDGETS) {
            _hLevels[_hLevelCount] = new HorizontalLevelWidget(id, *this);
            return *_hLevels[_hLevelCount++];
        }
        static HorizontalLevelWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_VERTICALLEVEL
    VerticalLevelWidget& vLevel(const char* id) {
        for (uint8_t i = 0; i < _vLevelCount; i++)
            if (strcmp(_vLevels[i]->getId(), id) == 0) return *_vLevels[i];
        if (_vLevelCount < INSTANTIOT_MAX_WIDGETS) {
            _vLevels[_vLevelCount] = new VerticalLevelWidget(id, *this);
            return *_vLevels[_vLevelCount++];
        }
        static VerticalLevelWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_ADVANCEDCHART
    AdvancedChartWidget& chart(const char* id) {
        for (uint8_t i = 0; i < _chartCount; i++)
            if (strcmp(_charts[i]->getId(), id) == 0) return *_charts[i];
        if (_chartCount < INSTANTIOT_MAX_WIDGETS) {
            _charts[_chartCount] = new AdvancedChartWidget(id, *this);
            return *_charts[_chartCount++];
        }
        static AdvancedChartWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_BARCHART
    BarChartWidget& barChart(const char* id) {
        for (uint8_t i = 0; i < _barChartCount; i++)
            if (strcmp(_barCharts[i]->getId(), id) == 0) return *_barCharts[i];
        if (_barChartCount < INSTANTIOT_MAX_WIDGETS) {
            _barCharts[_barChartCount] = new BarChartWidget(id, *this);
            return *_barCharts[_barChartCount++];
        }
        static BarChartWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    #ifdef INSTANTIOT_WIDGETS_TEXT
    TextWidget& text(const char* id) {
        for (uint8_t i = 0; i < _textCount; i++)
            if (strcmp(_texts[i]->getId(), id) == 0) return *_texts[i];
        if (_textCount < INSTANTIOT_MAX_WIDGETS) {
            _texts[_textCount] = new TextWidget(id, *this);
            return *_texts[_textCount++];
        }
        static TextWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    // ════════════════════════════════════════════════════════
    // ⚙️ CONFIG
    // ════════════════════════════════════════════════════════

    DeviceConfig& config() { return _config; }

protected:
    ITransport&  _transport;
    BinaryCodec  _codec;
    DeviceConfig _config;

    uint8_t _rxBuffer[INSTANT_RX_BUFFER_SIZE];
    size_t  _rxPos;
    uint8_t _txBuffer[INSTANT_TX_BUFFER_SIZE];

    bool _initialized;

    #ifdef INSTANTIOT_WIDGETS_LED
    LedWidget* _leds[INSTANTIOT_MAX_WIDGETS]; uint8_t _ledCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_GAUGE
    GaugeWidget* _gauges[INSTANTIOT_MAX_WIDGETS]; uint8_t _gaugeCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_METRIC
    MetricWidget* _metrics[INSTANTIOT_MAX_WIDGETS]; uint8_t _metricCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
    HorizontalLevelWidget* _hLevels[INSTANTIOT_MAX_WIDGETS]; uint8_t _hLevelCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_VERTICALLEVEL
    VerticalLevelWidget* _vLevels[INSTANTIOT_MAX_WIDGETS]; uint8_t _vLevelCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_ADVANCEDCHART
    AdvancedChartWidget* _charts[INSTANTIOT_MAX_WIDGETS]; uint8_t _chartCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_BARCHART
    BarChartWidget* _barCharts[INSTANTIOT_MAX_WIDGETS]; uint8_t _barChartCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_TEXT
    TextWidget* _texts[INSTANTIOT_MAX_WIDGETS]; uint8_t _textCount = 0;
    #endif

    // ════════════════════════════════════════════════════════
    // 📥 LECTURE — réassemblage trames binaires
    // ════════════════════════════════════════════════════════

    void readLoop() {
        while (_transport.available() > 0) {
            uint8_t buf[64];
            int n = _transport.read(buf, sizeof(buf));
            if (n <= 0) break;
            for (int i = 0; i < n; i++) {
                if (_rxPos < sizeof(_rxBuffer)) {
                    _rxBuffer[_rxPos++] = buf[i];
                } else {
                    _rxPos = 0;
                    IIOT_LOG("[Core] RX overflow, reset");
                }
            }
        }
        extractFrames();
    }

    void extractFrames() {
        // Trame minimale : AA(1) + VER(1) + LEN(2) + SEQ(1) + body(min0) + CRC(1) = 6
        while (_rxPos >= 5) {
            if (_rxBuffer[0] != 0xAA) { shiftBuffer(1); continue; }
            if (_rxBuffer[1] != 0x01) { shiftBuffer(1); continue; }

            uint16_t len = (uint16_t)_rxBuffer[2] | ((uint16_t)_rxBuffer[3] << 8);
            uint16_t frameSize = 5 + len + 1;  // ← header(5) + body + CRC

            if (_rxPos < frameSize) break;

            processFrame(_rxBuffer, frameSize);
            shiftBuffer(frameSize);
        }
    }

    void shiftBuffer(size_t n) {
        if (n >= _rxPos) { _rxPos = 0; return; }
        memmove(_rxBuffer, _rxBuffer + n, _rxPos - n);
        _rxPos -= n;
    }

    void processFrame(const uint8_t* data, size_t len) {
        DecodedMessage msg;
        uint8_t typeCode = 0, eventCode = 0;
        if (!_codec.decode(data, len, msg, typeCode, eventCode)) return;
        WidgetRegistry::dispatch(typeCode, msg.widgetId, eventCode, msg);
    }
};

} // namespace InstantIoT