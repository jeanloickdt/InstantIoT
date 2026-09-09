/*************************************************************
 * InstantIoT — connecting to the cloud, encrypted
 *
 * The board joins your WiFi then opens a TLS session to the InstantIoT
 * cloud. The token and the values never travel readable over the
 * internet.
 *
 * Replace before uploading:
 *   WIFI_SSID, WIFI_PASS   → your router
 *   DEVICE_TOKEN           → the token from the cloud panel
 *
 * Boards: ESP32, Uno R4 WiFi, MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2,
 * ESP8266. Only the ESP32 and the ESP8266 do the encryption themselves; the
 * others hand it to their WiFi module.
 *************************************************************/

// Not connecting? Uncomment this line BEFORE the include and reopen the
// serial monitor: the library then says which step it is stuck on — WiFi,
// TLS, or handshake. Without it, its logs are compiled out of the binary
// and only the messages below remain, which know nothing.
// #define INSTANTIOT_DEBUG 1

#include <InstantIoT.h>

const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";
const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

// The InstantIoT cloud, port 9443. For a staging deployment or a cloud
// you host, name the host and the port: see `.at(...)` below.
const char* SERVER_HOST  = "instantiot.cloud";
const uint16_t SERVER_PORT = 9443;

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;
bool lit = false;

void heartbeat() {
    lit = InstantIoT.connected() && !lit;
    digitalWrite(LED_BUILTIN, lit ? HIGH : LOW);
    Serial.println(InstantIoT.connected() ? "cloud: joined (TLS)" : "cloud: not joined");
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // `Cloud(...)` encrypts and verifies the server's identity against the
    // embedded Let's Encrypt roots. Three ways out, and they do not mean
    // the same thing:
    //
    //   .withCertificate(MY_ROOT)  your own authority — the identity is
    //                              still verified, against it
    //   .withoutCertCheck()        encrypted, but anyone can pose as the
    //                              server. For a first bring-up, not after
    //   .plaintext()               no encryption at all, for a board with
    //                              no TLS stack. The token then travels
    //                              readable, and that is the price
    Serial.print("target: "); Serial.print(SERVER_HOST);
    Serial.print(":");        Serial.println(SERVER_PORT);

    if (InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                         Cloud(DEVICE_TOKEN).at(SERVER_HOST, SERVER_PORT))) {
        Serial.print("Connected to the cloud. Local IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Not connected yet — the board keeps trying.");
    }

    timers.every(2000, heartbeat);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
