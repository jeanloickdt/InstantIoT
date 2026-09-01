/*************************************************************
 * InstantIoT — diagnosing a link, without a phone
 *
 * When nothing works the question is always the same: where does it stop?
 * This sketch answers with the board's LED and the serial monitor, before
 * a phone is even opened.
 *
 *   LED off          WiFi not joined
 *   LED steady       WiFi joined, server not
 *   LED blinking     server joined — all good
 *
 * Replace: WIFI_SSID, WIFI_PASS, SERVER_HOST, DEVICE_TOKEN.
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

void report() {
    // `WiFi` is the Arduino core's object: the question "am I on the
    // network?" is none of InstantIoT's business, and the facade does not
    // re-expose it. `InstantIoT.connected()` answers only for the link up
    // to the server.
    bool onNetwork = (WiFi.status() == WL_CONNECTED);
    bool onServer  = InstantIoT.connected();

    if (onServer) {
        lit = !lit;
        digitalWrite(LED_BUILTIN, lit ? HIGH : LOW);
        Serial.println("server joined");
        InstantIoT.write(I0, (long)(millis() / 1000));   // seconds since boot
    } else if (onNetwork) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("WiFi joined, server not — check address, port, token");
    } else {
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("WiFi not joined — check the network name and password");
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println();
    Serial.println("=== InstantIoT — diagnostic ===");
    Serial.print("server: "); Serial.println(SERVER_HOST);

    InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                     MyServer(SERVER_HOST, DEVICE_TOKEN));

    timers.every(1000, report);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
