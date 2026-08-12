/*************************************************************
 * InstantIoT — Example: Cloud Mode (TLS) - Secure Connect
 *
 * Use case: Verify ESP32 can connect to the InstantIoT Cloud
 *           over WiFi (Station) + **TLS** on port 9443.
 *           The token and all frames travel encrypted.
 *
 * Flow:
 *   1. WiFi STA connect
 *   2. TLS connect to host:9443 (server identity verified against
 *      the embedded Let's Encrypt roots)
 *   3. Handshake [LEN | "token:heartbeatMs"]  — default heartbeat 5000ms
 *   4. Library sends TYPE_HEARTBEAT frames periodically
 *   5. Local blink: built-in LED + log state every 2s
 *
 * Board : ESP32 only (does TLS comfortably)
 *
 * Before flashing, replace:
 *   WIFI_SSID, WIFI_PASS   → your router
 *   SERVER_HOST            → your cloud hostname (NOT a raw IP: the
 *                            hostname is needed for SNI + cert check)
 *   DEVICE_TOKEN           → token from the cloud panel
 *************************************************************/

#include <InstantIoTWiFiServerSecure.hpp>

// ----- WiFi -----
const char* WIFI_SSID = "MyWiFi";
const char* WIFI_PASS = "MyPassword";

// ----- InstantIoT Cloud (TLS) -----
const char* SERVER_HOST  = "instantiot.cloud";   // hostname, pas une IP
const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

#define LED_PIN 2  // Built-in LED on most ESP32 dev boards

// Port 9443 par défaut (portier TLS du cloud). Validation du serveur
// contre les racines Let's Encrypt embarquées.
InstantIoTWiFiServerSecure instant(SERVER_HOST, DEVICE_TOKEN);

uint32_t lastBlink = 0;
bool ledState = false;

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("=== InstantIoT Cloud Mode (TLS) - Secure Connect ===");
    Serial.print("Server:  "); Serial.print(SERVER_HOST); Serial.println(":9443 (TLS)");
    Serial.print("Token:   "); Serial.println(DEVICE_TOKEN);

    // Optional: override heartbeat (default 5000ms). Server offline
    // timeout ≈ 2.5× this value (clamped 2s..120s).
    // instant.setHeartbeat(5000);

    // Débogage seulement — chiffre sans vérifier l'identité du serveur.
    // À NE PAS laisser en prod (MITM possible) :
    // instant.setInsecure();

    // Serveur self-hosted avec un certificat maison :
    // instant.setCACert(MY_ROOT_CA_PEM);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] Failed to connect. Check serial for details.");
        // loop() keeps retrying (auto-reconnect with backoff)
    } else {
        Serial.print("Local IP: "); Serial.println(instant.getLocalIP());
        Serial.println("[OK] Connected to cloud over TLS");
    }
}

void loop() {
    instant.loop();

    if (millis() - lastBlink >= 2000) {
        lastBlink = millis();

        if (instant.connected()) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            Serial.println("[HB] connected (TLS) ✓");
        } else if (instant.isWiFiConnected()) {
            digitalWrite(LED_PIN, LOW);
            Serial.println("[HB] WiFi OK, waiting for cloud...");
        } else {
            digitalWrite(LED_PIN, LOW);
            Serial.println("[HB] WiFi down, reconnecting...");
        }
    }
}
