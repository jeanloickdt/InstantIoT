/*************************************************************
 * InstantIoT — a switch
 *
 * No `WHEN_` here, and that is not an oversight: only one thing can
 * happen to a switch — its value changes. So the head of the block
 * carries the data, and there is nothing to sort.
 *
 * The type you declare does the conversion: write `bool on`, and the
 * value arrives as a boolean.
 *
 * In the app: a switch on I0.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define RELAY_PIN 4

ISwitch(I0, bool on) {
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
    InstantIoT.write(I1, on ? "on" : "off");
};

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Switch", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
