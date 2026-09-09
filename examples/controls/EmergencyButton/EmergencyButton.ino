/*************************************************************
 * InstantIoT — the emergency stop
 *
 * Two states, and only one matters: tripped, or rearmed. While the stop
 * is active the board refuses to move — the sketch holds that rule, not
 * the app.
 *
 * In the app: an emergency button on I0.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define MOTOR_PIN 5

bool stopActive = false;

IEmergencyButton(I0) {
    WHEN_TRIGGERED {
        stopActive = true;
        analogWrite(MOTOR_PIN, 0);
        InstantIoT.write(I1, "STOPPED");
    }
    WHEN_RESET {
        stopActive = false;
        InstantIoT.write(I1, "ready");
    }
};

// A speed setpoint, ignored while the stop is active.
ISignal(I2, float speed) {
    if (stopActive) return;
    analogWrite(MOTOR_PIN, (int)(speed * 255.0f / 100.0f));
};

void setup() {
    Serial.begin(115200);
    pinMode(MOTOR_PIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Emergency", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
