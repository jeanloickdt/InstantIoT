/*************************************************************
 * TEST: DirectionPad
 * 
 * Events reçus de l'app:
 * - buttonpressed (payload: button = up/down/left/right/center)
 * - buttonlongpressed (payload: button)
 * - buttonreleased (payload: button)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_DPad", "12345678");

void onDirectionPadEvent(const DirectionPadEvent& e) {
    Serial.print("[DPad] id=");
    Serial.print(e.widgetId);
    Serial.print(" button=");
    Serial.print(e.buttonName);  // "up", "down", "left", "right", "center"
    
    // Press events
    ON_DPAD_UP("dpad1") { Serial.println(" UP_PRESS"); }
    ON_DPAD_DOWN("dpad1") { Serial.println(" DOWN_PRESS"); }
    ON_DPAD_LEFT("dpad1") { Serial.println(" LEFT_PRESS"); }
    ON_DPAD_RIGHT("dpad1") { Serial.println(" RIGHT_PRESS"); }
    ON_DPAD_CENTER("dpad1") { Serial.println(" CENTER_PRESS"); }
    
    // Long press events
    ON_DPAD_UP_LONG("dpad1") { Serial.println(" UP_LONG"); }
    ON_DPAD_DOWN_LONG("dpad1") { Serial.println(" DOWN_LONG"); }
    ON_DPAD_LEFT_LONG("dpad1") { Serial.println(" LEFT_LONG"); }
    ON_DPAD_RIGHT_LONG("dpad1") { Serial.println(" RIGHT_LONG"); }
    ON_DPAD_CENTER_LONG("dpad1") { Serial.println(" CENTER_LONG"); }
    
    // Release events
    ON_DPAD_UP_RELEASE("dpad1") { Serial.println(" UP_RELEASE"); }
    ON_DPAD_DOWN_RELEASE("dpad1") { Serial.println(" DOWN_RELEASE"); }
    ON_DPAD_LEFT_RELEASE("dpad1") { Serial.println(" LEFT_RELEASE"); }
    ON_DPAD_RIGHT_RELEASE("dpad1") { Serial.println(" RIGHT_RELEASE"); }
    ON_DPAD_CENTER_RELEASE("dpad1") { Serial.println(" CENTER_RELEASE"); }
}

void setup() {
        delay(3000);  // Attendre 3 secondes
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST: DirectionPad ===");
    Serial.println("Widget requis: DirectionPad id='dpad1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
