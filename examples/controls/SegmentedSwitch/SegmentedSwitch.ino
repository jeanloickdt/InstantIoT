/*************************************************************
 * InstantIoT — a segmented switch
 *
 * The app sends the index of the chosen segment, starting at 0. The board
 * does not know the labels: the app carries them, and that is exactly
 * right — changing them needs no reflashing.
 *
 * In the app: a segmented switch on I0, three segments
 * ("Eco", "Comfort", "Boost").
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define PWM_PIN 5

const int POWERS[] = { 60, 160, 255 };

ISegmentedSwitch(I0, int choice) {
    if (choice < 0 || choice > 2) return;   // an index outside the table does nothing
    analogWrite(PWM_PIN, POWERS[choice]);
    InstantIoT.write(I1, choice);
};

void setup() {
    Serial.begin(115200);
    pinMode(PWM_PIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Segmented", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
