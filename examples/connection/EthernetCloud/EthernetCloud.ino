/*
 * EthernetCloud — a board with no radio, reaching the cloud on a cable.
 *
 * Wiring: a W5100 / W5500 shield or module on the SPI bus. On a Mega that
 * is pins 50-53 plus the shield's own CS.
 *
 * ⚠️  PLAINTEXT, and it is not a shortcut. The W5x00 carries no crypto and
 * an AVR has neither the RAM nor the flash for a TLS handshake, so
 * `.plaintext()` is the only way this board reaches the cloud. The device
 * token then travels readable on the network — a decision on a home LAN,
 * a risk across the internet.
 *
 * ⚠️  A Mega, not an Uno. Measured: the library takes 61 % of an Uno's
 * 2 KB, and the Ethernet driver wants 388 bytes more. It does not fit.
 */

// BEFORE the include: the Arduino build discovers libraries by reading
// #include directives, so InstantIoT cannot pull in Ethernet on its own.
#define INSTANTIOT_ETHERNET 1
#include <InstantIoT.h>

const char* DEVICE_TOKEN = "paste-your-device-token-here";

InstantTimer timers;

// The app decides what I0 looks like — a gauge, a chart, a number.
void publish() {
    InstantIoT.write(I0, analogRead(A0) * 5.0 / 1023.0);
}

// A setpoint written from the app. It survives a reboot: the server
// replays it the moment this board reconnects.
ISignal(I1, float setpoint) {
    analogWrite(9, (int)(setpoint * 255.0 / 100.0));
};

void setup() {
    Serial.begin(115200);
    pinMode(9, OUTPUT);

    // No SSID, no password: a cable is plugged in or it is not.
    if (InstantIoT.begin(EthernetLink(), Cloud(DEVICE_TOKEN).plaintext())) {
        Serial.println(F("Connected."));
    } else {
        // Normal on the first pass: DHCP may not have answered yet.
        // loop() keeps trying, with a growing delay.
        Serial.println(F("Not connected yet — the board keeps trying."));
    }

    timers.every(2000, publish);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
