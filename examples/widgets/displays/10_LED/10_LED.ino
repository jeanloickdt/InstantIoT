/*************************************************************
 * InstantIoT — Example: LED
 *
 * Use case: Visual indicator for temperature status
 *
 * Widget: LED  id="led1"
 * Board : ESP8266 / ESP32
 *
 * Wiring (real sensor — optional):
 *   DHT22 → DATA_PIN (GPIO4)
 *   DS18B20 → DATA_PIN (GPIO4)
 *
 * Behavior:
 *   Reads temperature every second.
 *   temp < 30°C  → Green,  brightness 50%
 *   30..48°C     → Orange, brightness proportional
 *   > 48°C       → Red,    brightness 100%
 *
 * Protocol (Device → App):
 *   setbrightness(0..100), setcolor(R,G,B)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

// ── Uncomment your sensor ────────────────────────────────────
// #include <DHT.h>
// #define DATA_PIN 4
// DHT dht(DATA_PIN, DHT22);

InstantIoTWiFiAP instant("InstantIoT_LED", "12345678");
InstantTimer timers;

// ── Temperature source ───────────────────────────────────────

float readTemperature() {
    // ── Real sensor (uncomment one) ──────────────────────────
    // return dht.readTemperature();       // DHT22
    // return sensors.getTempCByIndex(0);  // DS18B20

    // ── Simulation — rises from 20°C to 55°C then resets ────
    static float temp = 20.0f;
    temp += 0.5f;
    if (temp > 55.0f) temp = 20.0f;
    return temp;
}

// ── LED update ───────────────────────────────────────────────

void updateLED() {
    if (!instant.connected()) return;

    float temp = readTemperature();
    Serial.print("Temp: "); Serial.print(temp); Serial.println("°C");

    if (temp < 30.0f) {
        // Green — normal
        instant.led("led1").setColor(0x00FF00);
        instant.led("led1").setBrightness(50);

    } else if (temp <= 48.0f) {
        // Orange — brightness proportional to heat (0..100%)
        uint8_t brightness = (uint8_t)((temp - 30.0f) / 18.0f * 100.0f);
        instant.led("led1").setColor(0xFF8800);
        instant.led("led1").setBrightness(brightness);

    } else {
        // Red — hot, full brightness
        instant.led("led1").setColor(0xFF0000);
        instant.led("led1").setBrightness(100);
    }
}

// ── Setup / Loop ─────────────────────────────────────────────

void setup() {
    delay(2000);
    Serial.begin(115200);
    // dht.begin();

    Serial.println("\n=== LED Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: LED  id='led1'");

    instant.led("led1").Off();

    timers.every(1000, updateLED);
}

void loop() {
    instant.loop();
    timers.run();
}