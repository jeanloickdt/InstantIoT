/*************************************************************
 * InstantIoT — the board is the network
 *
 * No router, no account, no token: the board opens its own WiFi network,
 * the phone joins it, and the app talks to the board directly. This is
 * "Direct" in the app, and the shortest path to a first working button.
 *
 * On the phone:  New project → InstantIoT Direct → your board → Connect.
 * The app lists the networks nearby; pick the one named below.
 *
 * Replace before uploading:
 *   AP_NAME   → the network name the phone will see
 *   AP_PASS   → eight characters or more, or the network does not start
 *
 * Boards: ESP32, ESP8266, Uno R4 WiFi, MKR WiFi 1010, Nano 33 IoT,
 * Uno WiFi Rev.2.
 *************************************************************/

// Not connecting? Uncomment this line BEFORE the include and reopen the
// serial monitor: the library then says what it is doing.
// #define INSTANTIOT_DEBUG 1

#include <InstantIoT.h>

const char* AP_NAME = "MyBoard";
const char* AP_PASS = "12345678";   // your own, 8+ characters

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

// One button in the app, at address I0, drives the LED.
ISimpleButton(I0) {
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW);  }
};

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // `AccessPoint(...)` alone: there is no destination to name, the phone
    // is at the other end of the wire. To move the same sketch to the
    // cloud later, this is the only line that changes:
    //   InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
    InstantIoT.begin(AccessPoint(AP_NAME, AP_PASS));

    Serial.print("Network: "); Serial.println(AP_NAME);
    Serial.println("Join it from the phone, then Connect in the app.");
}

void loop() {
    InstantIoT.loop();
}
