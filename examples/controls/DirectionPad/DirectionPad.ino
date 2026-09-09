/*************************************************************
 * InstantIoT — the direction pad
 *
 * Here the `WHEN_` clauses earn their place: the keys do different
 * things, and naming them avoids the staircase of `if`.
 *
 * Two ways to read, and they do not serve the same purpose:
 *
 *   WHEN_UP { … }              one key, one gesture
 *   WHEN_PAD_PRESSED(key)      every key, same treatment
 *
 * The five pad keys are named. The other eight — A, B, X, Y and the four
 * shapes — go through the variable form.
 *
 * In the app: a direction pad on I0.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define LEFT_MOTOR  5
#define RIGHT_MOTOR 18

static void drive(int left, int right) {
    analogWrite(LEFT_MOTOR,  left);
    analogWrite(RIGHT_MOTOR, right);
}

IDirectionPad(I0) {
    WHEN_UP      { drive(200, 200); }
    WHEN_DOWN    { drive(100, 100); }
    WHEN_LEFT    { drive(0,   200); }
    WHEN_RIGHT   { drive(200, 0);   }

    WHEN_UP_LONG { drive(255, 255); }   // full throttle while held

    // Any key released stops everything: which one is almost never
    // interesting.
    WHEN_RELEASED_ANY { drive(0, 0); }

    // The action keys, handled together.
    WHEN_PAD_PRESSED(key) {
        if (key == DPadButton::A) InstantIoT.write(I1, "horn");
        if (key == DPadButton::B) InstantIoT.write(I1, "lights");
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(LEFT_MOTOR,  OUTPUT);
    pinMode(RIGHT_MOTOR, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Pad", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
