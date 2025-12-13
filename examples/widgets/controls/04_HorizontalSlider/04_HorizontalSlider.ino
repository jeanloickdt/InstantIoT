/*************************************************************
 * TEST: HorizontalSlider
 * 
 * Events reçus de l'app (payload: value)
 * - valuechanged
 * - valuechanging
 * - dragstarted (startValue)
 * - dragended (finalValue)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_HSlider", "12345678");

void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
    Serial.print("[HSlider] id=");
    Serial.print(e.widgetId);
    
    ON_VALUE_CHANGING("slider1") {
        Serial.print(" VALUE_CHANGING=");
        Serial.println(e.value);
    }

}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: HorizontalSlider ===");
    Serial.println("Widget requis: HorizontalSlider id='slider1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
