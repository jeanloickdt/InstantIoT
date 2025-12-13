/*************************************************************
 * TEST: Metric Widget
 * 
 * Events envoyés vers l'app:
 * - setvalue (payload: value)
 * - setunit (payload: unit)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_Metric", "12345678");
InstantTimer timers;

float temperature = 20.0;

void testMetric() {
    if (!instant.connected()) return;
    
    temperature +=10;
    if(temperature> 200)
        temperature =0;
    
    Serial.print("Metric: setValue(");
    Serial.print(temperature);
    Serial.println(")");
    
    instant.metric("metric1")
        .setValue(temperature);
       
}

void setup() {
     delay(3000);
    Serial.begin(115200);
    delay(1000);
    randomSeed(analogRead(0));
    Serial.println("\n=== TEST: Metric Widget ===");
    Serial.println("Widget requis: Metric id='metric1'");
    instant.begin();
    
    timers.every(500, testMetric);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
