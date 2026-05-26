#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

/**
 * BarChart — display of N values as bars.
 *
 * **One-way direction**: Arduino → App. The widget is purely
 * downstream: the ESP32 pushes the values, the app displays them. Bar
 * labels and colors are configured **on the app side**
 * (settings sheet) — the user defines the number of "slots" and
 * their look, and the sketch sends an array of N values in the
 * same order.
 *
 * Typical example:
 *   float values[3];
 *   values[0] = readTemperature();
 *   values[1] = readHumidity();
 *   values[2] = readPressure();
 *   instant.barChart("env").setValues(values, 3);
 *
 * Memory: ~16 bytes per instance + 4×count in buffer for
 * setValues. No dynamic allocation.
 */
class BarChartWidget : public DisplayWidget {
public:
    BarChartWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_BARCHART; }

    /**
     * Pushes a complete array of values. The app replaces all known
     * bars with these values (order = order of the slots
     * configured in the settings sheet).
     *
     * @param values pointer to `count` floats
     * @param count number of values (must be ≥ 1, max 64
     *              to fit in the codec's 256B buffer)
     */
    BarChartWidget& setValues(const float* values, uint8_t count) {
        if (count == 0 || values == nullptr) return *this;
        if (count > 64) count = 64;  // cap to stay within buffer

        uint8_t buf[1 + 64 * 4];
        buf[0] = count;
        for (uint8_t i = 0; i < count; i++) {
            writeFloatLE(buf + 1 + i * 4, values[i]);
        }
        sendBinary(EV_BAR_SETVALUES, buf, 1 + count * 4);
        return *this;
    }

    /**
     * Updates a **single** bar by its index (0-indexed).
     * More bandwidth-efficient than `setValues` when only one
     * value changes. If `index` does not match any slot configured
     * on the app side, the frame is silently ignored.
     */
    BarChartWidget& setBar(uint8_t index, float value) {
        uint8_t buf[5];
        buf[0] = index;
        writeFloatLE(buf + 1, value);
        sendBinary(EV_BAR_SETBAR, buf, 5);
        return *this;
    }

    /**
     * Resets all bars to 0 on the app side. Useful on a clean boot
     * or on user reset.
     */
    BarChartWidget& clear() {
        sendBinary(EV_BAR_CLEAR);
        return *this;
    }
};

} // namespace InstantIoT
