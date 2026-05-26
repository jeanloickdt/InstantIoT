/*************************************************************
 * InstantIoT — Example: Server Mode - Basic Connect
 *
 * Use case: Verify ESP32 can connect to a remote InstantIoT
 *           Server over WiFi (Station) + TCP.
 *
 * Flow:
 *   1. WiFi STA connect
 *   2. TCP connect to server:port
 *   3. Handshake [LEN | "token:heartbeatMs"]  — default heartbeat 5000ms
 *   4. Library sends a TYPE_HEARTBEAT frame (0xFE) every `heartbeat` ms
 *      so the server knows the device is alive even when idle.
 *   5. Local blink: built-in LED + log state every 2s
 *
 * Board : ESP32 only (for now)
 *
 * Before flashing, replace:
 *   WIFI_SSID, WIFI_PASS       → your router
 *   SERVER_IP, SERVER_PORT     → your InstantIoT Server
 *   DEVICE_TOKEN               → token from server admin panel
 *************************************************************/

#include <InstantIoTWiFiServer.hpp>

// ----- WiFi -----
const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";

// ----- InstantIoT Server -----
const char* SERVER_IP    = "192.168.1.42";
const uint16_t SERVER_PORT = 9001;
const char* DEVICE_TOKEN = "991c7874-aff6-4ed1-a707-b8ce6ba149c4";

#define LED_PIN 2  // Built-in LED on most ESP32 dev boards

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);

uint32_t lastHeartbeat = 0;
bool ledState = false;

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("=== InstantIoT Server Mode - Basic Connect ===");
    Serial.print("Server:  "); Serial.print(SERVER_IP);
    Serial.print(":");         Serial.println(SERVER_PORT);
    Serial.print("Token:   "); Serial.println(DEVICE_TOKEN);

    // Optional: override the heartbeat interval (default 5000ms).
    // The server adapts its offline detection timeout to ≈ 2.5× this
    // value (clamped between 2s and 120s). 0 = disable (legacy mode).
    // instant.setHeartbeat(5000);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] Failed to connect. Check serial for details.");
        // loop() will keep trying (auto-reconnect with backoff)
    } else {
        Serial.print("Local IP: "); Serial.println(instant.getLocalIP());
        Serial.println("[OK] Connected to server");
    }
}

void loop() {
    instant.loop();

    // Heartbeat every 2s
    if (millis() - lastHeartbeat >= 2000) {
        lastHeartbeat = millis();

        if (instant.connected()) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            Serial.println("[HB] connected ✓");
        } else if (instant.isWiFiConnected()) {
            digitalWrite(LED_PIN, LOW);
            Serial.println("[HB] WiFi OK, waiting for server...");
        } else {
            digitalWrite(LED_PIN, LOW);
            Serial.println("[HB] WiFi down, reconnecting...");
        }
    }
}
