/*************************************************************
 * TEST: Gauge Widget
 * 
 * Events envoyés vers l'app:
 * - setvalue (payload: value)
 * - setrange (payload: min, max)
 * - setunit (payload: unit)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_Gauge", "12345678");
InstantTimer timers;

float value = 0;

void testGauge() {
    if (!instant.connected()) return;
    
    value += 50;
    if (value > 500) value = 0;
    
    Serial.print("Gauge: setValue(");
    Serial.print(value);
    Serial.println(")");
    
    instant.gauge("gauge1").update(value, 0, 500);
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST: Gauge Widget ===");
    Serial.println("Widget requis: Gauge id='gauge1'");
    instant.begin();
    
    timers.every(1000, testGauge);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
