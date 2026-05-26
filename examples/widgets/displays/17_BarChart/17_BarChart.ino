/*************************************************************
 * InstantIoT — Example: BarChart
 *
 * Use case: Multi-sensor environmental dashboard
 *
 * Widget: BarChart  id="env"
 * Board : ESP8266 / ESP32
 *
 * Behavior:
 *   Reads 3 simulated sensors (temperature, humidity, pressure)
 *   and pushes the 3 values as an array of bars every
 *   2 seconds. On the app side, the user has configured 3 slots
 *   in the settings sheet — label + color — each slot
 *   receives the corresponding value by index.
 *
 * Slots on the app side (configured once):
 *   #0 → Temp     (red, 0..40)
 *   #1 → Humidity (blue, 0..100)
 *   #2 → Pressure (yellow, 980..1040)
 *
 * Protocol (Device → App):
 *   setValues(values, count)   — push the whole array
 *   setBar(index, value)       — update a single bar
 *   clear()                    — reset
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("InstantIoT_BarChart", "12345678");
InstantTimer timers;

// ── Simulated sensors ────────────────────────────────────────
// Replace with real BME280 / DHT22 / etc. readings.
float readTemperature() { return 20.0f + random(-30, 30) / 10.0f; }
float readHumidity()    { return 45.0f + random(-50, 50) / 10.0f; }
float readPressure()    { return 1013.0f + random(-50, 50) / 10.0f; }

// ── BarChart update ──────────────────────────────────────────

void updateBarChart() {
    if (!instant.connected()) return;

    // The order of values must match the order of slots
    // configured in the app (widget settings sheet).
    float values[3];
    values[0] = readTemperature();
    values[1] = readHumidity();
    values[2] = readPressure();

    instant.barChart("env").setValues(values, 3);
}

// ── Setup / Loop ─────────────────────────────────────────────

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== BarChart Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: BarChart  id='env'  3 bars (Temp / Humid / Pressure)");

    timers.every(2000, updateBarChart);
}

void loop() {
    instant.loop();
    timers.run();
}
