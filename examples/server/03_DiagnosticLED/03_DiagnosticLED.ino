/*************************************************************
 * InstantIoT — Example: Server Mode - DIAGNOSTIC LED
 *
 * Purpose: Pure visual diagnostic — no Serial needed.
 *          Use this to prove whether frames are arriving
 *          at the ESP32 when the Serial Monitor is broken.
 *
 * Widgets (create in app with these exact IDs):
 *   - SimpleButton       id="btn1"
 *   - HorizontalSlider   id="dim1"
 *   - Gauge              id="sim1"  (optional, for TX side)
 *
 * LED patterns (all on LED_PIN = built-in):
 *
 *   Heartbeat  → 50ms pulse every 2s       (proves ESP is alive)
 *   Press      → 3 rapid blinks            (btn1 Press arrived)
 *   Release    → 1 rapid blink             (btn1 Release arrived)
 *   LongPress  → 5 rapid blinks            (btn1 LongPress arrived)
 *   Slider     → LED ON for 200ms          (dim1 ValueChanged arrived)
 *
 * If you see NO reaction to the app → frames aren't reaching the ESP.
 * If you see heartbeat only       → TCP connected but frames dropped.
 * If you see event blinks          → ESP receives correctly, the bug
 *                                     was elsewhere (Android or server).
 *
 * Board : ESP32 only
 *************************************************************/

#include <InstantIoTWiFiServer.hpp>
#include <utils/InstantIoTTimer.hpp>
#include <math.h>

// ----- WiFi -----
const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";

// ----- InstantIoT Server -----
const char* SERVER_IP    = "192.168.1.42";
const uint16_t SERVER_PORT = 9001;
const char* DEVICE_TOKEN = "991c7874-aff6-4ed1-a707-b8ce6ba149c4";

#define LED_PIN 2   // Built-in LED on most ESP32 dev boards

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);
InstantTimer timers;

// Blink N times quickly (~80ms on, 80ms off). Blocking.
void blinkN(int n) {
    for (int i = 0; i < n; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(80);
        digitalWrite(LED_PIN, LOW);
        delay(80);
    }
}

// ── Button: visible patterns ─────────────────────────────────
void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1")      { blinkN(3); }
    ON_RELEASE("btn1")    { blinkN(1); }
    ON_LONG_PRESS("btn1") { blinkN(5); }
    // Toggle ignored for this test
}

// ── Slider: solid flash for 200ms ────────────────────────────
void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
    ON_VALUE_CHANGED("dim1", value) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
    }
    // ValueChanging ignored (too fast — would lock up the LED)
}

// ── Heartbeat: short pulse every 2s ──────────────────────────
void heartbeat() {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
}

// ── Optional gauge push (same as example 02) ─────────────────
float simPhase = 0.0f;
void pushGauge() {
    if (!instant.connected()) return;
    simPhase += 0.1f;
    if (simPhase > TWO_PI) simPhase -= TWO_PI;
    float value = (sinf(simPhase) + 1.0f) * 50.0f;
    instant.gauge("sim1").update(value, 0.0f, 100.0f);
}

// ── Setup / Loop ──────────────────────────────────────────────
void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 3 slow blinks at boot → tells you a new firmware is running
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH); delay(200);
        digitalWrite(LED_PIN, LOW);  delay(200);
    }

    instant.begin(WIFI_SSID, WIFI_PASS);

    // Heartbeat every 2s once begin() returned
    timers.every(2000, heartbeat);
    // Optional gauge every 500ms (remove if you don't need sim1)
    timers.every(500, pushGauge);
}

void loop() {
    instant.loop();
    timers.run();
}
