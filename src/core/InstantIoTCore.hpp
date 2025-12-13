#pragma once
/**
 * ============================================================
 * 🔄 InstantIoTCore.hpp - Boucle principale
 * ============================================================
 */

#include <Arduino.h>
#include "Transport.h"
#include "JsonCodec.hpp"
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
    // 🔧 LIFECYCLE
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

    // ════════════════════════════════════════════════════════
    // 📊 WIDGET ACCESS
    // ════════════════════════════════════════════════════════

    #ifdef INSTANTIOT_WIDGETS_LED
    LedWidget& led(const char* id) {
        for (uint8_t i = 0; i < _ledCount; i++) {
            if (strcmp(_leds[i]->getId(), id) == 0) return *_leds[i];
        }
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
        for (uint8_t i = 0; i < _gaugeCount; i++) {
            if (strcmp(_gauges[i]->getId(), id) == 0) return *_gauges[i];
        }
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
        for (uint8_t i = 0; i < _metricCount; i++) {
            if (strcmp(_metrics[i]->getId(), id) == 0) return *_metrics[i];
        }
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
        for (uint8_t i = 0; i < _hLevelCount; i++) {
            if (strcmp(_hLevels[i]->getId(), id) == 0) return *_hLevels[i];
        }
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
        for (uint8_t i = 0; i < _vLevelCount; i++) {
            if (strcmp(_vLevels[i]->getId(), id) == 0) return *_vLevels[i];
        }
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
        for (uint8_t i = 0; i < _chartCount; i++) {
            if (strcmp(_charts[i]->getId(), id) == 0) return *_charts[i];
        }
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
        for (uint8_t i = 0; i < _barChartCount; i++) {
            if (strcmp(_barCharts[i]->getId(), id) == 0) return *_barCharts[i];
        }
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
        for (uint8_t i = 0; i < _textCount; i++) {
            if (strcmp(_texts[i]->getId(), id) == 0) return *_texts[i];
        }
        if (_textCount < INSTANTIOT_MAX_WIDGETS) {
            _texts[_textCount] = new TextWidget(id, *this);
            return *_texts[_textCount++];
        }
        static TextWidget dummy("__dummy__", *this);
        return dummy;
    }
    #endif

    // ════════════════════════════════════════════════════════
    // 📤 ENVOI
    // ════════════════════════════════════════════════════════

    bool sendMessage(const char* widgetId, const char* widgetType, const char* event, const char* payload = nullptr) override {
        if (!_transport.connected()) return false;

        size_t len = _codec.encode(
            _txBuffer, sizeof(_txBuffer),
            _config.getDashboardId(), _config.getDeviceId(),
            widgetId, widgetType, event, payload
        );

        if (len == 0) return false;

        if (len < sizeof(_txBuffer) - 1) {
            _txBuffer[len++] = '\n';
        }

        return _transport.write((const uint8_t*)_txBuffer, len) == len;
    }

    // ════════════════════════════════════════════════════════
    // ⚙️ CONFIG
    // ════════════════════════════════════════════════════════

    DeviceConfig& config() { return _config; }

protected:
    ITransport& _transport;
    JsonCodec _codec;
    DeviceConfig _config;

    char _rxBuffer[INSTANT_RX_BUFFER_SIZE];
    size_t _rxPos;
    char _txBuffer[INSTANT_TX_BUFFER_SIZE];

    bool _initialized;

    #ifdef INSTANTIOT_WIDGETS_LED
    LedWidget* _leds[INSTANTIOT_MAX_WIDGETS];
    uint8_t _ledCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_GAUGE
    GaugeWidget* _gauges[INSTANTIOT_MAX_WIDGETS];
    uint8_t _gaugeCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_METRIC
    MetricWidget* _metrics[INSTANTIOT_MAX_WIDGETS];
    uint8_t _metricCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_HORIZONTALLEVEL
    HorizontalLevelWidget* _hLevels[INSTANTIOT_MAX_WIDGETS];
    uint8_t _hLevelCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_VERTICALLEVEL
    VerticalLevelWidget* _vLevels[INSTANTIOT_MAX_WIDGETS];
    uint8_t _vLevelCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_ADVANCEDCHART
    AdvancedChartWidget* _charts[INSTANTIOT_MAX_WIDGETS];
    uint8_t _chartCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_BARCHART
    BarChartWidget* _barCharts[INSTANTIOT_MAX_WIDGETS];
    uint8_t _barChartCount = 0;
    #endif
    #ifdef INSTANTIOT_WIDGETS_TEXT
    TextWidget* _texts[INSTANTIOT_MAX_WIDGETS];
    uint8_t _textCount = 0;
    #endif

    // ════════════════════════════════════════════════════════
    // 📥 LECTURE EN CHUNKS
    // ════════════════════════════════════════════════════════

    void readLoop() {
        while (_transport.available() > 0) {
            uint8_t buf[64];
            int n = _transport.read(buf, sizeof(buf));
            if (n <= 0) break;

            for (int i = 0; i < n; i++) {
                uint8_t c = buf[i];

                if (c == '\n') {
                    if (_rxPos > 0) {
                        _rxBuffer[_rxPos] = '\0';
                        processLine(_rxBuffer, _rxPos);
                        _rxPos = 0;
                    }
                } else if (c != '\r') {
                    if (_rxPos < sizeof(_rxBuffer) - 1) {
                        _rxBuffer[_rxPos++] = c;
                    }
                }
            }
        }
    }

    void processLine(const char* data, size_t len) {
        if (len == 0 || data[0] != '{') return;

        DecodedMessage msg;
        if (!_codec.decode(data, len, msg)) return;

        WidgetRegistry::dispatch(msg.widgetType, msg.widgetId, msg.event, msg);
    }
};

} // namespace InstantIoT