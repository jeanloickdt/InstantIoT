/*************************************************************
 * TEST: Switch
 * 
 * Events reçus de l'app:
 * - turnon
 * - turnoff
 * - toggle
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_Switch", "12345678");

void onSwitchEvent(const SwitchEvent& e) {
    ON_TURN_ON("sw1") {
        Serial.println("ON");
    }
    ON_TURN_OFF("sw1") {
        Serial.println("OFF");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: Switch ===");
    Serial.println("Widget requis: Switch id='sw1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
