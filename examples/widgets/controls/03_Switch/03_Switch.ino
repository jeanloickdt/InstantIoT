/*************************************************************
 *InstantIoT — Example: Switch
 *
 * Use case: Control a relay with a switch
 *
 * Widget: Switch  id="sw1"
 * Board : ESP8266 / ESP32
 *
 * Wiring:
 *   RELAY_PIN (GPIO5) → Relay module → Load
 *
 * Behavior:
 *   Switch ON  → Relay ON  → LED built-in ON
 *   Switch OFF → Relay OFF → LED built-in OFF
 *
 * Protocol (App → Device):
 *   turnon, turnoff, toggle, setvalue(isOn)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

#define LED_PIN   2  // Built-in LED (active LOW on ESP8266)
#define RELAY_PIN 5  // Relay module

InstantIoTWiFiAP instant("InstantIoT_Switch", "12345678");

bool relayState = false;

void setRelay(bool on) {
    relayState = on;
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
    digitalWrite(LED_PIN,   on ? LOW  : HIGH); // built-in active LOW
    Serial.print("Relay → "); Serial.println(on ? "ON" : "OFF");
}

void onSwitchEvent(const SwitchEvent& e) {
    ON_TURN_ON("sw1") {
        setRelay(true);
        Serial.println("sw1 → TURN ON");
    }

    ON_TURN_OFF("sw1") {
        setRelay(false);
        Serial.println("sw1 → TURN OFF");
    }

    ON_SWITCH_TOGGLE("sw1") {
        setRelay(e.isOn);
        Serial.println("sw1 → TOGGLE");
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN,   HIGH);
    pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);

    Serial.println("\n=== Switch Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: Switch  id='sw1'");
}

void loop() {
    instant.loop();
}
