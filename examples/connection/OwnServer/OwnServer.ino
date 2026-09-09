/*************************************************************
 * InstantIoT — connecting to YOUR server
 *
 * The shortest sketch that talks to an InstantIoT server you host: the
 * board joins your WiFi, reaches the server, and the LED blinks while the
 * link holds.
 *
 * Replace before uploading:
 *   WIFI_SSID, WIFI_PASS   → your router
 *   SERVER_HOST            → your server's IP or hostname
 *   DEVICE_TOKEN           → the board token
 *
 * Boards: ESP32, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";

const char* SERVER_HOST  = "192.168.1.42";
const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;
bool lit = false;

// The LED beat, once a second. `timers.every` rather than a hand-copied
// `millis() - last >= …`, and above all not a `delay()`: that would also
// stop reading the frames coming in.
void heartbeat() {
    lit = InstantIoT.connected() && !lit;
    digitalWrite(LED_BUILTIN, lit ? HIGH : LOW);
    Serial.println(InstantIoT.connected() ? "server: joined" : "server: not joined");
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // The link, then the destination: the board joins a network first,
    // then reaches a server. Plaintext, because a server at your place
    // does not leave your place. In front of a TLS server:
    //   MyServer(SERVER_HOST, DEVICE_TOKEN).secure()
    if (InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                         MyServer(SERVER_HOST, DEVICE_TOKEN))) {
        Serial.print("Connected. Local IP: ");
        Serial.println(WiFi.localIP());
    } else {
        // Failing here is normal: at boot the router is not always ready.
        // `loop()` keeps trying, with a growing delay.
        Serial.println("Not connected yet — the board keeps trying.");
    }

    timers.every(2000, heartbeat);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
