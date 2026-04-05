/*************************************************************
 * InstantIoT — Example 13: AdvancedChart
 *
 * Use case: Plot distance over time with HC-SR04
 *
 * Widget: AdvancedChart  id="chart1"
 * Board : ESP32
 *
 * Wiring:
 *   HC-SR04 TRIG → GPIO14
 *   HC-SR04 ECHO → GPIO12
 *   HC-SR04 VCC  → 5V
 *   HC-SR04 GND  → GND
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

#define TRIG_PIN  14
#define ECHO_PIN  12
#define DIST_MAX 200.0f

InstantIoTWiFiAP instant("InstantIoT_Chart", "12345678");
InstantTimer timers;

float readDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1.0f;
    return duration * 0.034f / 2.0f;
}

void updateChart() {
    if (!instant.connected()) return;

    float distance = readDistance();
    if (distance < 0 || distance > DIST_MAX) return;

    // Add point to series "distance"
    instant.chart("chart1").addPoint("distance", distance);

    Serial.print("Distance: "); Serial.print(distance);
    Serial.println(" cm");
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    instant.begin();
    timers.every(500, updateChart);

    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== AdvancedChart Example ===");
}

void loop() {
    instant.loop();
    timers.run();
}