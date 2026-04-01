/*************************************************************
 * InstantIoT — Example: Gauge
 *
 * Use case: Distance / tank level monitor
 *
 * Widget: Gauge  id="tank"
 * Board : ESP8266 (Lolin) / ESP32
 *
 * Wiring (HC-SR04):
 *   VCC  → 3.3V
 *   GND  → GND
 *   TRIG → D5 (GPIO14)
 *   ECHO → D6 (GPIO12)
 *
 * No voltage divider needed when powered at 3.3V.
 *
 * Behavior:
 *   Reads tank level every 2 seconds.
 *   Sends value (0..100%) + range to app.
 *   Comment #define USE_HCSR04 to use simulation instead.
 *
 * Protocol (Device → App):
 *   update(value, min, max)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

// ── Sensor config ────────────────────────────────────────────
// Comment/uncomment to switch between real and simulated sensor
#define USE_HCSR04

#ifdef USE_HCSR04
    #define TRIG_PIN 14  // D5
    #define ECHO_PIN 12  // D6
    #define TANK_HEIGHT_CM 100.0f  // Full tank depth in cm
    #define TANK_MIN_CM     5.0f  // Sensor dead zone
#endif

InstantIoTWiFiAP instant("InstantIoT_Gauge", "12345678");
InstantTimer timers;

// ── Tank level reading ────────────────────────────────────────

float readTankLevel() {
#ifdef USE_HCSR04
    // HC-SR04 measurement
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1.0f;  // timeout — no echo

    float distanceCm = duration * 0.034f / 2.0f;
    float level = ((TANK_HEIGHT_CM - distanceCm) /
                   (TANK_HEIGHT_CM - TANK_MIN_CM)) * 100.0f;
    return constrain(level, 0.0f, 100.0f);
#else
    // Simulation — slow drain then refill
    static float level = 100.0f;
    level -= 1.5f;
    if (level < 0.0f) level = 100.0f;
    return level;
#endif
}

// ── Gauge update ──────────────────────────────────────────────

void updateGauge() {
    if (!instant.connected()) return;

    float level = readTankLevel();

    if (level < 0.0f) {
        Serial.println("Sensor timeout — no reading");
        return;
    }

   // Serial.print("Tank level: ");
    //Serial.print(level); 
    //Serial.println("%");
    instant.gauge("tank").update(level, 0.0f, 100.0f);
}

// ── Setup / Loop ─────────────────────────────────────────────

void setup() {
    delay(2000);
    Serial.begin(115200);

#ifdef USE_HCSR04
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("Sensor: HC-SR04");
#else
    Serial.println("Sensor: Simulated");
#endif

    Serial.println("\n=== Gauge Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: Gauge  id='tank'  range: 0..100%");

    timers.every(2000, updateGauge);
}

void loop() {
    instant.loop();
    timers.run();
}