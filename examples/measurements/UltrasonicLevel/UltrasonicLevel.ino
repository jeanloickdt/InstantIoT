/*************************************************************
 * InstantIoT — a tank level, with an ultrasonic sensor
 *
 * This sketch replaces four older examples: gauge, horizontal level,
 * vertical level, chart. They all had the same code — the same sensor,
 * the same `write` — and differed only in the drawing, which no longer
 * belongs to the board.
 *
 * The board writes a number. What is made of it is decided in the app,
 * and changed without reflashing.
 *
 * In the app, on I0: a gauge. Or a chart, or a metric, or a level — it is
 * the same signal.
 *
 * Wiring: HC-SR04, TRIG on 14, ECHO on 12 (through a divider if the board
 * is 3.3 V).
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define TRIG_PIN 14
#define ECHO_PIN 12

#define TANK_HEIGHT_CM 100.0f   // full depth
#define DEAD_ZONE_CM     5.0f   // the sensor sees nothing closer

InstantTimer timers;

/** @return the level as a percentage, or -1 if no echo came back. */
float tankLevel() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return -1.0f;

    float distanceCm = duration * 0.034f / 2.0f;
    float level = ((TANK_HEIGHT_CM - distanceCm) /
                   (TANK_HEIGHT_CM - DEAD_ZONE_CM)) * 100.0f;
    return constrain(level, 0.0f, 100.0f);
}

void publish() {
    float level = tankLevel();
    // A failed reading is not sent: the last known value beats an
    // invented zero.
    if (level >= 0.0f) InstantIoT.write(I0, level);
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Tank", "12345678"));

    // One reading a second: an ultrasonic sensor has nothing more to say
    // more often, and the tank does not empty that fast. `every` replaces
    // the `millis() - last >= …` everyone ends up copying wrong, and
    // `delay()` would not do: it would also stop reading incoming frames.
    timers.every(1000, publish);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
