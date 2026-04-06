/*************************************************************
 * InstantIoT — Example: Joystick
 *
 * Use case: Control a servo + LEDs with joystick
 *
 * Widget: Joystick  id="joy1"
 * Board : ESP32
 *
 * Wiring:
 *   Servo   → GPIO13 (signal jaune/orange)
 *   LED R   → GPIO25 → 220Ω → GND
 *   LED G   → GPIO26 → 220Ω → GND
 *   LED Y   → GPIO27 → 220Ω → GND
 *
 * Behavior:
 *   X axis  → servo angle 0..180°
 *   Y > 0   → LED verte (avant)
 *   Y < 0   → LED rouge (arrière)
 *   Neutre  → LED jaune
 *   Release → servo centre + LED jaune
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <ESP32Servo.h>

#define SERVO_PIN 13
#define LED_R     25
#define LED_G     26
#define LED_Y     27

InstantIoTWiFiAP instant("InstantIoT_Joystick", "12345678");

Servo servo;

void setServo(float x) {
    int angle = (int)((x + 1.0f) * 90.0f);
    angle = constrain(angle, 0, 180);
    servo.write(angle);
}

void setLED(float y) {
    if (y > 0.1f) {
        int brightness = (int)(y * 255.0f);
        analogWrite(LED_R, 0);
        analogWrite(LED_G, brightness);
        analogWrite(LED_Y, 0);
    } else if (y < -0.1f) {
        int brightness = (int)(-y * 255.0f);
        analogWrite(LED_R, brightness);
        analogWrite(LED_G, 0);
        analogWrite(LED_Y, 0);
    } else {
        analogWrite(LED_R, 0);
        analogWrite(LED_G, 0);
        analogWrite(LED_Y, 50);
    }
}

void onJoystickEvent(const JoystickEvent& e) {
    ON_JOYSTICK("joy1", x, y) {
        setServo(x);
        setLED(y);
        Serial.print("x="); Serial.print(x);
        Serial.print(" y="); Serial.println(y);
    }

    ON_JOYSTICK_RELEASED("joy1") {
        servo.write(90);
        analogWrite(LED_R, 0);
        analogWrite(LED_G, 0);
        analogWrite(LED_Y, 50);
        Serial.println("Released — center");
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_Y, OUTPUT);

    servo.attach(SERVO_PIN);
    servo.write(90);

    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_Y, 50);

    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== Joystick Example ===");
}

void loop() {
    instant.loop();
}