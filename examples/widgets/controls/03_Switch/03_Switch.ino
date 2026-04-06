/*************************************************************
 * InstantIoT — Example 14: Switch
 *
 * Use case: Control a relay with a switch
 *
 * Widget: Switch  id="sw1"
 * Board : ESP32
 *
 * Wiring:
 *   Relay → GPIO4
 *   LED   → GPIO2 (built-in, active LOW)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

#define RELAY_PIN 4
#define LED_PIN   2

InstantIoTWiFiAP instant("InstantIoT_Switch", "12345678");

void onSwitchEvent(const SwitchEvent& e) {

      Serial.println(e.isOn);
    ON_TURN_ON("sw1") {
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, LOW);
        Serial.println("sw1 → ON");
    }

    ON_TURN_OFF("sw1") {
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, HIGH);
        Serial.println("sw1 → OFF");
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);   digitalWrite(LED_PIN, HIGH);

    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== Switch Example ===");
}

void loop() {
    instant.loop();
}