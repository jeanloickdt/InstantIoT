/*************************************************************
 * InstantIoT — a horizontal slider
 *
 * The slider writes a value, the board receives it. Nothing to sort: the
 * head of the block carries the data.
 *
 * Mind the flow: a sliding finger produces a lot of values. The library
 * caps the rate, but the sketch must do nothing slow inside this block.
 *
 * In the app: a horizontal slider on I0, bounded 0 to 100.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define PWM_PIN 5

IHorizontalSlider(I0, float value) {
    analogWrite(PWM_PIN, (int)(value * 255.0f / 100.0f));
};

void setup() {
    Serial.begin(115200);
    pinMode(PWM_PIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_HSlider", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
