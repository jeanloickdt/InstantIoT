/*************************************************************
 * InstantIoT — two wires and a cheap module
 *
 * For a board with no radio of its own: an HC-05, HC-06, HM-10, JDY-30 or
 * any module that speaks over a serial line does the talking, and
 * `SerialLink` hands it the bytes. The app connects to the module.
 *
 * This is the link an Arduino Uno or a Nano can use — the ones that have
 * neither WiFi nor an Ethernet shield.
 *
 * Boards: AVR (Uno, Nano, Mega) and ESP8266. SoftwareSerial belongs to
 * those cores; the ESP32 does not have it, and does not need it.
 *
 * Wiring:
 *   module RX  → TX_PIN of the board   (voltage divider on a 5 V board)
 *   module TX  → RX_PIN of the board
 *   module VCC → 3.3 V or 5 V, per the module
 *   module GND → GND
 *
 * SoftwareSerial is reliable up to about 57600 baud. 9600 is the default
 * of most of these modules and of this library.
 *************************************************************/

#include <InstantIoT.h>

// GPIO 10 and 11 are wired to the flash chip on most ESP8266 modules, so
// the pins are not the same on the two families. On a Mega, the RX pin
// must be interrupt-capable: 10 is, many others are not — check the
// SoftwareSerial documentation before moving it.
#if defined(ESP8266)
  #define RX_PIN 13   // D7
  #define TX_PIN 15   // D8
#else
  #define RX_PIN 10
  #define TX_PIN 11
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 13
#endif

InstantTimer timers;

// Same block as everywhere else. Nine thousand six hundred bauds or a TLS
// session to the cloud: a signal does not know how it travelled.
ISimpleButton(I0) {
    WHEN_PRESSED       { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW);  }
};

void sendReading() {
    InstantIoT.write(I1, analogRead(A0));
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    InstantIoT.begin(SerialLink(RX_PIN, TX_PIN));

    // Every 5 seconds, not every 2: at 9600 baud the line is narrow, and a
    // frame that is still going out when the next one starts is a frame
    // nobody reads.
    timers.every(5000, sendReading);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
