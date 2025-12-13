/*************************************************************
 * TEST: VerticalLevel Widget
 * 
 * Events envoyés vers l'app:
 * - setvalue (payload: value)
 * - setrange (payload: min, max)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_VLevel", "12345678");
InstantTimer timers;

float level = 50;

void testVLevel() {
    if (!instant.connected()) return;
    
    // Simulation réservoir qui se vide
    level -= 2;
    if (level < 0) level = 100;
    
    Serial.print("VLevel: setValue(");
    Serial.print(level);
    Serial.println(")");
    
    instant.vLevel("vlevel1")
        .setValue(level)
        .setRange(0, 100);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: VerticalLevel Widget ===");
    Serial.println("Widget requis: VerticalLevel id='vlevel1'");
    instant.begin();
    
    timers.every(300, testVLevel);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
