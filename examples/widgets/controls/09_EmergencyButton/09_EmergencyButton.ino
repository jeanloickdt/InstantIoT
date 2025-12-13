/*************************************************************
 * TEST: EmergencyButton
 * 
 * Events reçus de l'app:
 * - trigger
 * - reset
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_Emergency", "12345678");

void onEmergencyButtonEvent(const EmergencyButtonEvent& e) {
    Serial.print("[Emergency] id=");
    Serial.print(e.widgetId);
    
    ON_TRIGGER("emer1") {
        Serial.println(" TRIGGER!");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: EmergencyButton ===");
    Serial.println("Widget requis: EmergencyButton id='emer1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
