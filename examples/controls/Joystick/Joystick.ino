/*************************************************************
 * InstantIoT — a joystick
 *
 * The app sends a position — two numbers between -1 and 1 — and the block
 * receives them already decoded. Returning to centre arrives as a
 * position too: (0, 0).
 *
 * In the app: a joystick on I0.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define LEFT_MOTOR  5
#define RIGHT_MOTOR 18

IJoystick(I0, float x, float y) {
    // The classic mix: forward comes from y, turning from x.
    float left  = y + x;
    float right = y - x;
    analogWrite(LEFT_MOTOR,  (int)(constrain(left,  0.0f, 1.0f) * 255));
    analogWrite(RIGHT_MOTOR, (int)(constrain(right, 0.0f, 1.0f) * 255));
};

void setup() {
    Serial.begin(115200);
    pinMode(LEFT_MOTOR,  OUTPUT);
    pinMode(RIGHT_MOTOR, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Joystick", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
