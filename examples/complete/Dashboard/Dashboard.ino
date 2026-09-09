/*************************************************************
 * InstantIoT — a small dashboard
 *
 * Six signals, both directions, and nothing more than an ordinary sketch:
 * a slider setting an intensity, a button lighting up, a switch
 * commuting, and three values published.
 *
 * What the example really shows: the board does not know what the app
 * draws. It receives values at addresses, and writes others. The
 * dashboard can be rearranged without reflashing.
 *
 * Signals to declare in the app:
 *   received  I0 slider    I1 button   I2 switch
 *   written   I5 intensity  I6 uptime of the light  I7 state in words
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
#define PWM_PIN   5
#define RELAY_PIN 4

int      intensity = 0;
bool     relayClosed = false;
uint32_t litSince = 0;
InstantTimer timers;

IHorizontalSlider(I0, float value) {
    intensity = (int)(value * 255.0f / 100.0f);
    analogWrite(PWM_PIN, intensity);
};

ISimpleButton(I1) {
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); litSince = millis(); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW);  }
};

ISwitch(I2, bool closed) {
    relayClosed = closed;
    digitalWrite(RELAY_PIN, closed ? HIGH : LOW);
};

void publish() {
    InstantIoT.write(I5, intensity * 100 / 255);
    InstantIoT.write(I6, litSince ? (millis() - litSince) / 1000 : 0);
    InstantIoT.write(I7, relayClosed ? "relay closed" : "relay open");
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PWM_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Dashboard", "12345678"));
    timers.every(1000, publish);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
