/*************************************************************
 * TEST: HorizontalLevel & VerticalLevel Widgets
 * 
 * Events envoyés vers l'app:
 * - update (payload: value, min, max)
 *************************************************************/


#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_Levels", "12345678");
InstantTimer timers;

float levelH = 0;
float levelV = 0;
int directionH = 1;
int directionV = 1;

void testHLevel() {
    if (!instant.connected()) return;
    
    levelH += directionH * 5;
    if (levelH >= 200) directionH = -1;
    if (levelH <= 0) directionH = 1;
    
    Serial.print("HLevel: update(");
    Serial.print(levelH);
    Serial.println(", 0, 200)");
    
    instant.hLevel("hl1").update(levelH, 0, 200);
}

void testVLevel() {
    if (!instant.connected()) return;
    
    levelV += directionV * 3;
    if (levelV >= 100) directionV = -1;
    if (levelV <= 0) directionV = 1;
    
    Serial.print("VLevel: update(");
    Serial.print(levelV);
    Serial.println(", 0, 100)");
    
    instant.vLevel("vl1").update(levelV, 0, 100);
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== TEST: Level Widgets ===");
    Serial.println("Widgets requis:");
    Serial.println("  - HorizontalLevel id='hl1'");
    Serial.println("  - VerticalLevel id='vl1'");
    
    instant.begin();
    
    timers.every(200, testHLevel);
    timers.every(210, testVLevel);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}