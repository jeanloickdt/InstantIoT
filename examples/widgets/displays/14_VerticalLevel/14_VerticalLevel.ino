/*************************************************************
 * InstantIoT — Example 11: VerticalLevel
 *
 * Use case: Display tank level with HC-SR04
 *
 * Widget: VerticalLevel  id="vl1"
 * Board : ESP32
 *
 * Wiring:
 *   HC-SR04 TRIG → GPIO14
 *   HC-SR04 ECHO → GPIO12
 *   HC-SR04 VCC  → 5V
 *   HC-SR04 GND  → GND
 *
 * Behavior:
 *   Distance 2cm  → Level 100% (full)
 *   Distance 40cm → Level 0%   (empty)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

#define TRIG_PIN  14
#define ECHO_PIN  12
#define DIST_MIN  2.0f
#define DIST_MAX 40.0f

InstantIoTWiFiAP instant("InstantIoT_Level", "12345678");
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

void updateLevel() {
    if (!instant.connected()) return;

    float distance = readDistance();
    if (distance < 0 || distance > DIST_MAX) return;

    // Close = full, far = empty
    float level = (DIST_MAX - distance) / (DIST_MAX - DIST_MIN) * 100.0f;
    level = constrain(level, 0.0f, 100.0f);

    instant.vLevel("vl1").update(level, 0.0f, 100.0f);

    Serial.print("Distance: "); Serial.print(distance);
    Serial.print(" cm | Level: "); Serial.print(level);
    Serial.println("%");
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    instant.begin();
    timers.every(300, updateLevel);

    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== VerticalLevel Example ===");
}

void loop() {
    instant.loop();
    timers.run();
}