/*************************************************************
 * InstantIoT — Example 09: Metric
 *
 * Use case: Display temperature with min/max range
 *
 * Widgets:
 *   Metric  id="metric1"
 *     - Primary value  : current temperature (float)
 *     - Secondary value: min / max range (string)
 *
 * Board : ESP32
 *
 * Note:
 *   Configure unit "°C" and decimals in the app widget settings.
 *   Enable "Show Secondary" to display the Min/Max row.
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("InstantIoT_Metric", "12345678");

InstantTimer timers;

float temperature = 20.0f;
float minTemp     = 20.0f;
float maxTemp     = 20.0f;

void updateMetric() {
    if (!instant.connected()) return;

    // Simulate temperature variation
    temperature += random(-10, 10) / 10.0f;
    temperature  = constrain(temperature, -10.0f, 50.0f);

    // Track min / max
    if (temperature < minTemp) minTemp = temperature;
    if (temperature > maxTemp) maxTemp = temperature;

    // Send primary value
    instant.metric("metric1").setValue(temperature);

    // Send secondary value — min/max range as string
    char stats[32];
    snprintf(stats, sizeof(stats), "%.1f / %.1f", minTemp, maxTemp);
    instant.metric("metric1").setSecondaryValue(stats, "Min / Max");

    Serial.print("Temp: "); Serial.print(temperature);
    Serial.print(" | Min: "); Serial.print(minTemp);
    Serial.print(" | Max: "); Serial.println(maxTemp);
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    instant.begin();
    timers.every(500, updateMetric);

    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== Metric Example ===");
}

void loop() {
    instant.loop();
    timers.run();
}