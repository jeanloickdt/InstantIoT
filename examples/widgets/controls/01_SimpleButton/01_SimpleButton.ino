/*************************************************************
 * TEST: SimpleButton
 * 
 * Events reçus de l'app:
 * - press
 * - release  
 * - longpress
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_SimpleButton", "12345678");

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    Serial.print("[SimpleButton] id=");
    Serial.print(e.widgetId);
    Serial.print(" event=");
    
    ON_PRESS("btn1") {
        Serial.println("PRESS");
    }
    ON_RELEASE("btn1") {
        Serial.println("RELEASE");
    }
    ON_LONG_PRESS("btn1") {
        Serial.println("LONG_PRESS");
    }
    ON_TOGGLE_STATE("btn1", var_isOn){
        Serial.println(var_isOn ? "ON" : "OFF");
    }
}

void setup() {
    delay(3000);  // Attendre 3 secondes
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== TEST: SimpleButton ===");
    Serial.println("Widget requis: SimpleButton id='btn1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
