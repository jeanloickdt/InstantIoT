/*************************************************************
 * InstantIoT — text and a state
 *
 * Not everything is measured. A board also has things to SAY — which mode
 * it is in, what it has just done — and states to show.
 *
 * A signal carries at most 48 characters. It is a value, not a log: what
 * does not fit here had no business being here.
 *
 * In the app, on I0: a text display. On I1: an LED. On I2: a metric.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

const char* MODES[] = { "Eco", "Comfort", "Boost" };
int   currentMode = 0;
long  cyclesDone = 0;
InstantTimer timers;

// The app chooses the mode; the board sends it back in words, so the
// display shows what the board UNDERSTOOD and not what the phone sent.
ISegmentedSwitch(I3, int choice) {
    if (choice < 0 || choice > 2) return;
    currentMode = choice;
    InstantIoT.write(I0, MODES[currentMode]);
};

void publish() {
    cyclesDone++;
    InstantIoT.write(I0, MODES[currentMode]);
    InstantIoT.write(I1, currentMode == 2);     // "Boost" lights the LED
    InstantIoT.write(I2, cyclesDone);
}

void setup() {
    Serial.begin(115200);
    InstantIoT.begin(AccessPoint("InstantIoT_Text", "12345678"));
    timers.every(3000, publish);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
