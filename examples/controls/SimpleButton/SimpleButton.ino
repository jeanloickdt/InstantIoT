/*************************************************************
 * InstantIoT — a simple button
 *
 * Three gestures, three blocks, and not one `if` to write.
 *
 * In the app: put a simple button on signal I0 of this board. The
 * gesture travels as a value — 1 press, 0 release, 2 long press — and
 * the block turns it back into a gesture.
 *
 * Boards: ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

ISimpleButton(I0) {
    WHEN_PRESSED       { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW);  }
    WHEN_LONG_PRESSED  {
        // Answering from inside the block is the normal case: the board
        // tells the app the long press was understood.
        InstantIoT.write(I1, "long press");
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Button", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
