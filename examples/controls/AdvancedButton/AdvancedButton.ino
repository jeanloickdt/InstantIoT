/*************************************************************
 * InstantIoT — an advanced button
 *
 * The same vocabulary as the simple button. What changes is in the app:
 * the look, the label, the colour. The board receives exactly the same
 * thing.
 *
 * In the app: an advanced button on I0.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

int presses = 0;

IAdvancedButton(I0) {
    WHEN_PRESSED {
        presses++;
        digitalWrite(LED_BUILTIN, HIGH);
        InstantIoT.write(I1, presses);      // a counter, to display
    }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW); }
    WHEN_LONG_PRESSED  { presses = 0; InstantIoT.write(I1, presses); }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_AdvButton", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
