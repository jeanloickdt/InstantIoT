/*************************************************************
 * InstantIoT — Example: DirectionPad
 *
 * Use case: Control LEDs with direction pad
 *
 * Widget: DirectionPad  id="dpad1"
 * Board : ESP32
 *
 * Wiring:
 *   LED R  → GPIO25 → 220Ω → GND
 *   LED G  → GPIO26 → 220Ω → GND
 *   LED Y  → GPIO27 → 220Ω → GND
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

#define LED_R 25
#define LED_G 26
#define LED_Y 27

InstantIoTWiFiAP instant("InstantIoT_DPad", "12345678");

void onDirectionPadEvent(const DirectionPadEvent& e) {

    Serial.print("[DPad] button=");
    Serial.print(e.buttonName);
    Serial.print(" kind=");
    Serial.println((int)e.kind);
    

    ON_DPAD_UP("dpad1")     { digitalWrite(LED_G, HIGH); }
    ON_DPAD_DOWN("dpad1")   { digitalWrite(LED_R, HIGH); }
    ON_DPAD_LEFT("dpad1")   { digitalWrite(LED_Y, HIGH); }
    ON_DPAD_RIGHT("dpad1")  { digitalWrite(LED_Y, HIGH); }
    ON_DPAD_CENTER("dpad1") {
        digitalWrite(LED_R, HIGH);
        digitalWrite(LED_G, HIGH);
        digitalWrite(LED_Y, HIGH);
    }

    ON_DPAD_UP_RELEASE("dpad1")    { digitalWrite(LED_G, LOW); }
    ON_DPAD_DOWN_RELEASE("dpad1")  { digitalWrite(LED_R, LOW); }
    ON_DPAD_LEFT_RELEASE("dpad1")  { digitalWrite(LED_Y, LOW); }
    ON_DPAD_RIGHT_RELEASE("dpad1") { digitalWrite(LED_Y, LOW); }
    ON_DPAD_CENTER_RELEASE("dpad1") {
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_G, LOW);
        digitalWrite(LED_Y, LOW);
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_Y, OUTPUT);
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}