/*************************************************************
 * InstantIoT Example: Simple Button -> LED
 * 
 * Le plus simple exemple possible:
 * - Button "btn1" contrôle la LED intégrée ESP32 (GPIO 2)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

#define LED_PIN 2

InstantIoTWiFiAP instant("MyESP32", "12345678");

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED ON");
    }
    
    ON_RELEASE("btn1") {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED OFF");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("\n=== InstantIoT Basic Example ===");
    
    if (instant.begin()) {
        Serial.print("WiFi: MyESP32 | Pass: 12345678 | IP: ");
        Serial.println(instant.getIP());
    }
}

void loop() {
    instant.loop();
}
