/*************************************************************
 * InstantIoT — Example: Server Mode - Slider + Button + Gauge
 *
 * Use case: Full round-trip test with 3 widgets over the
 *           InstantIoT Server (ESP32 as TCP client).
 *
 * Widgets (create them in the app with these exact IDs):
 *   - SimpleButton       id="btn1"   (App → Device)
 *   - HorizontalSlider   id="dim1"   (App → Device)
 *   - Gauge              id="sim1"   (Device → App)
 *
 * Flow:
 *   - btn1 pressed/released  → LED ON/OFF
 *   - dim1 moves             → LED PWM brightness (0..100%)
 *   - sim1                   → simulated sine wave (0..100)
 *                               pushed every 500ms
 *
 * Board : ESP32 only
 *
 * Replace WIFI_*, SERVER_*, DEVICE_TOKEN before flashing.
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

#define LED_PIN 2          // Built-in LED on most ESP32 dev boards
#define PWM_FREQ    5000
#define PWM_RES     8      // 0..255

// ESP32 Core 3.x unified API (ledcAttach) vs 2.x (ledcSetup + channel)
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
  #define PWM_CHANNEL 0
  #define PWM_INIT()  do { ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES); \
                           ledcAttachPin(LED_PIN, PWM_CHANNEL); } while(0)
  #define PWM_WRITE(v) ledcWrite(PWM_CHANNEL, (v))
#else
  #define PWM_INIT()   ledcAttach(LED_PIN, PWM_FREQ, PWM_RES)
  #define PWM_WRITE(v) ledcWrite(LED_PIN, (v))
#endif

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);
InstantTimer timers;

// ── State ─────────────────────────────────────────────────────
bool  buttonHeld  = false;
float brightness  = 0.0f;   // %, from slider
float simPhase    = 0.0f;

// ── Helpers ───────────────────────────────────────────────────
void applyBrightness() {
    // button overrides slider: if button held → full ON
    float effective = buttonHeld ? 100.0f : brightness;
    int pwm = (int)(effective / 100.0f * 255.0f);
    PWM_WRITE(pwm);
}

// ── Button events ─────────────────────────────────────────────
void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        buttonHeld = true;
        applyBrightness();
        Serial.println("btn1 → PRESS");
    }
    ON_RELEASE("btn1") {
        buttonHeld = false;
        applyBrightness();
        Serial.println("btn1 → RELEASE");
    }
    ON_LONG_PRESS("btn1") {
        Serial.println("btn1 → LONG PRESS");
    }
}

// ── Slider events ─────────────────────────────────────────────
void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
    ON_VALUE_CHANGING("dim1", value) {
        brightness = value;
        applyBrightness();
    }
    ON_VALUE_CHANGED("dim1", value) {
        brightness = value;
        applyBrightness();
        Serial.print("dim1 → "); Serial.print(value); Serial.println("%");
    }
}

// ── Gauge push (simulated sensor) ─────────────────────────────
void pushGauge() {
    if (!instant.connected()) return;

    // simple 0..100 sine wave, one full cycle every ~10s
    simPhase += 0.1f;
    if (simPhase > TWO_PI) simPhase -= TWO_PI;
    float value = (sinf(simPhase) + 1.0f) * 50.0f;  // 0..100

    instant.gauge("sim1").update(value, 0.0f, 100.0f);
}

// ── Setup / Loop ──────────────────────────────────────────────
void setup() {
    delay(2000);
    Serial.begin(115200);

    // ESP32 PWM on LED_PIN (works on ESP32 Core 2.x and 3.x)
    PWM_INIT();
    PWM_WRITE(0);

    Serial.println();
    Serial.println("=== Server Mode - Slider/Button/Gauge ===");
    Serial.print("Server: "); Serial.print(SERVER_IP);
    Serial.print(":");        Serial.println(SERVER_PORT);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] Initial connect failed, will keep retrying.");
    } else {
        Serial.print("Local IP: "); Serial.println(instant.getLocalIP());
        Serial.println("[OK] Connected");
    }

    Serial.println("Widgets expected: btn1 (SimpleButton), dim1 (HSlider), sim1 (Gauge)");

    timers.every(500, pushGauge);
}

void loop() {
    instant.loop();
    timers.run();
}
