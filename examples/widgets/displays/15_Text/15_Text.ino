/*************************************************************
 * InstantIoT — Example 12: Text
 *
 * Use case: Display dynamic text from ESP32
 *
 * Widget: Text  id="txt1"
 * Board : ESP32
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("InstantIoT_Text", "12345678");
InstantTimer timers;

int counter = 0;

void updateText() {
    if (!instant.connected()) return;

    char msg[32];
    snprintf(msg, sizeof(msg), "Count: %d", counter++);
    instant.text("txt1").setText(msg);

    Serial.println(msg);
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    instant.begin();
    timers.every(1000, updateText);

    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("=== Text Example ===");
}

void loop() {
    instant.loop();
    timers.run();
}