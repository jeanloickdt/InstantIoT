/*************************************************************
 * TEST: AdvancedButton
 * 
 * Events reçus de l'app:
 * - press
 * - release
 * - longpress
 * - toggle (avec isOn)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_AdvButton", "12345678");

void onAdvancedButtonEvent(const AdvancedButtonEvent& e) {
    Serial.print("[AdvancedButton] id=");
    Serial.print(e.widgetId);
    
    ON_PRESS("btn1") {
        Serial.println(" PRESS");
    }
    ON_RELEASE("btn1") {
        Serial.println(" RELEASE");
    }
    ON_LONG_PRESS("btn1") {
        Serial.println(" LONG_PRESS");
    }
    ON_TOGGLE_STATE("btn1", isOn) {
        Serial.print(" TOGGLE isOn=");
        Serial.println(isOn ? "true" : "false");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: AdvancedButton ===");
    Serial.println("Widget requis: AdvancedButton id='btn1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
