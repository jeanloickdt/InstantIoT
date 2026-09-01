/*************************************************************
 * InstantIoT — a temperature, from an analog sensor
 *
 * An LM35 or a TMP36 on an analog input: three wires, no library to
 * install. That is the reason for the choice — the older examples needed
 * the DHT library fetched before they would even compile.
 *
 * In the app, on I0: a metric, or a chart, or a gauge. On I1: an LED —
 * it lights when it gets too warm.
 *
 * Wiring: sensor output on A0 (pin 34 on ESP32).
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#if defined(ESP32)
  #define SENSOR_PIN 34
#else
  #define SENSOR_PIN A0
#endif

float alertThreshold = 30.0f;   // changeable from the app
InstantTimer timers;

/** LM35: 10 mV per degree. TMP36: add the 0.5 V offset. */
float temperature() {
    float volts = analogRead(SENSOR_PIN) * 3.3f / 4095.0f;
    return volts * 100.0f;
}

// The threshold is a STATE, not a gesture: on reboot the server sends it
// back and this block finds it again, with nobody opening the app.
ISignal(I2, float threshold) {
    alertThreshold = threshold;
};

void publish() {
    float t = temperature();
    InstantIoT.write(I0, t);
    InstantIoT.write(I1, t > alertThreshold);
}

void setup() {
    Serial.begin(115200);
    InstantIoT.begin(AccessPoint("InstantIoT_Temperature", "12345678"));
    timers.every(2000, publish);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
