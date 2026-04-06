#include <InstantIoTWiFiAP.hpp>

#define LED_PIN 2

InstantIoTWiFiAP instant("InstantIoT_Slider", "12345678");

void applyBrightness(float value) {
    int pwm = (int)(value / 100.0f * 255.0f);
    analogWrite(LED_PIN, pwm);
    Serial.print("Brightness: "); Serial.print(value); Serial.println("%");
}

void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
    ON_VALUE_CHANGING("dim1", value) { applyBrightness(value); }
    ON_VALUE_CHANGED("dim1", value)  { applyBrightness(value); }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}