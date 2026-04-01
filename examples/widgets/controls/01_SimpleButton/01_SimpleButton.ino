/*************************************************************
 * InstantIoT — Example: SimpleButton
 *
 * Use case: Control a LED with a button
 *
 * Widget: SimpleButton  id="btn1"
 * Board : ESP8266 / ESP32
 *
 * Wiring:
 *   LED_PIN (GPIO2) → 220Ω → LED → GND
 *
 * Behavior:
 *   Press      → LED ON
 *   Release    → LED OFF
 *   Long Press → Blink 3x
 *
 * Protocol (App → Device):
 *   press, release, longpress, toggle(isOn)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

#define LED_PIN 2  // Built-in LED (active LOW on ESP8266)

InstantIoTWiFiAP instant("InstantIoT_Button", "12345678");

bool isBlinking = false;

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        digitalWrite(LED_PIN, LOW);   // ON
        Serial.println("btn1 → PRESS — LED ON");
    }

    ON_RELEASE("btn1") {
        if (!isBlinking) {
            digitalWrite(LED_PIN, HIGH);  // OFF
            Serial.println("btn1 → RELEASE — LED OFF");
        }
    }

    ON_LONG_PRESS("btn1") {
        Serial.println("btn1 → LONG PRESS — Blink 3x");
        isBlinking = true;
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, LOW);  delay(150);
            digitalWrite(LED_PIN, HIGH); delay(150);
        }
        isBlinking = false;
    }

    ON_TOGGLE_STATE("btn1", isOn) {
        digitalWrite(LED_PIN, isOn ? LOW : HIGH);
        Serial.print("btn1 → TOGGLE — isOn=");
        Serial.println(isOn);
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // OFF

    Serial.println("\n=== SimpleButton Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: SimpleButton  id='btn1'");
}

void loop() {
    instant.loop();
}
