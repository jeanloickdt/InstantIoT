/*************************************************************
 * TEST: Joystick
 * 
 * Events reçus de l'app:
 * - positionchanged (payload: x, y) float -1.0 à 1.0
 * - released
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_Joystick", "12345678");

void onJoystickEvent(const JoystickEvent& e) {
    Serial.print("[Joystick] id=");
    Serial.print(e.widgetId);
    
    ON_JOYSTICK("joy1", x, y) {
        Serial.print(" POSITION x=");
        Serial.print(x);
        Serial.print(" y=");
        Serial.println(y);
    }

}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: Joystick ===");
    Serial.println("Widget requis: Joystick id='joy1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
