/*************************************************************
 * TEST: VerticalSlider
 * 
 * Events reçus de l'app (payload: value)
 * - valuechanged
 * - valuechanging
 * - dragstarted
 * - dragended
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_VSlider", "12345678");

void onVerticalSliderEvent(const VerticalSliderEvent& e) {
    Serial.print("[VSlider] id=");
    Serial.print(e.widgetId);
    
    ON_VALUE_CHANGING("slider1") {
        Serial.print(" VALUE_CHANGING=");
        Serial.println(e.value);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: VerticalSlider ===");
    Serial.println("Widget requis: VerticalSlider id='slider1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
