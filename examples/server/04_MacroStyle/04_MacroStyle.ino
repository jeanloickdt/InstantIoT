/*************************************************************
 * InstantIoT — Example: Macro-style declarative handlers
 *
 * Démontre la syntaxe ISimpleButton / IAdvancedButton / IJoystick
 * avec les WHEN_* (C++17+). Chaque bloc est déclaré au scope
 * fichier — aucun `void onXxxEvent(...)` à écrire.
 *
 * Widgets à créer dans l'app avec ces IDs :
 *   - SimpleButton     id="btn1"   (TOGGLE mode)
 *   - SimpleButton     id="btn2"   (MOMENTARY mode)
 *   - AdvancedButton   id="relay1"
 *   - Joystick         id="joy1"
 *
 * Board : ESP32
 *************************************************************/

#include <InstantIoTWiFiServer.hpp>

const char* WIFI_SSID      = "YOUR_WIFI_SSID";
const char* WIFI_PASS      = "YOUR_WIFI_PASSWORD";
const char* SERVER_IP      = "192.168.1.42";
const uint16_t SERVER_PORT = 9001;
const char* DEVICE_TOKEN   = "PASTE_YOUR_DEVICE_TOKEN";

#define LED_PIN   2
#define RELAY_PIN 4

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);

// ── Bouton btn1 (TOGGLE) — pilote la LED via WHEN_TOGGLED ────
ISimpleButton("btn1") {
    WHEN_TOGGLED(isOn) {
        analogWrite(LED_PIN, isOn ? 255 : 0);
        Serial.print("btn1 toggled → ");
        Serial.println(isOn ? "ON" : "OFF");
    }
};

// ── Bouton btn2 (MOMENTARY) — press/release/longpress ───────
ISimpleButton("btn2") {
    WHEN_PRESSED       { Serial.println("btn2 pressed"); }
    WHEN_RELEASED      { Serial.println("btn2 released"); }
    WHEN_LONG_PRESSED  { Serial.println("btn2 long-pressed"); }
};

// ── AdvancedButton relay1 — coexistence type distinct ───────
IAdvancedButton("relay1") {
    WHEN_TOGGLED(on) {
        digitalWrite(RELAY_PIN, on ? HIGH : LOW);
        Serial.print("relay1 → "); Serial.println(on ? "ON" : "OFF");
    }
};

// ── Joystick joy1 — WHEN_MOVED capture x/y, WHEN_RELEASED ───
IJoystick("joy1") {
    WHEN_MOVED(x, y) {
        Serial.print("joy1 → x="); Serial.print(x, 1);
        Serial.print(" y=");       Serial.println(y, 1);
    }
    WHEN_RELEASED {
        Serial.println("joy1 released (kill switch)");
    }
};

// ── Setup / Loop ────────────────────────────────────────────
void setup() {
    delay(2000);
    Serial.begin(115200);
    instant.setHeartbeat(1000);

    pinMode(LED_PIN,   OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    analogWrite(LED_PIN, 0);
    digitalWrite(RELAY_PIN, LOW);

    Serial.println();
    Serial.println("=== Macro-style handlers demo ===");
    Serial.print("Server: "); Serial.print(SERVER_IP);
    Serial.print(":");        Serial.println(SERVER_PORT);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] Initial connect failed, will keep retrying.");
    } else {
        Serial.print("Local IP: "); Serial.println(instant.getLocalIP());
        Serial.println("[OK] Connected");
    }

    Serial.println("Widgets: btn1 (toggle), btn2 (momentary), relay1 (adv-toggle), joy1");
}

void loop() {
    instant.loop();
}
