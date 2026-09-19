/*************************************************************
 * InstantIoT — two wires and a cheap module
 *
 * For a board with no radio of its own: an HC-05, HC-06, HM-10, JDY-30 or
 * any module that speaks over a serial line does the talking, and
 * `SerialLink` hands it the bytes. The app connects to the module.
 *
 * Two roads onto the board, and the sketch picks the right one:
 *
 *   SerialLink(Serial1)      a hardware UART — Uno R4 WiFi, MKR, Nano 33
 *                            IoT, ESP32, and the Mega's spare UARTs
 *   SerialLink(RX, TX)       two software pins — Uno, Nano, ESP8266,
 *                            whose only UART is the USB one
 *
 * Wiring (hardware UART, Serial1):
 *   module RX  → TX1 of the board   (voltage divider on a 5 V board)
 *   module TX  → RX1 of the board
 *   module VCC → 3.3 V or 5 V, per the module
 *   module GND → GND
 *
 * Wiring (software pins):
 *   module RX  → TX_PIN of the board   (voltage divider on a 5 V board)
 *   module TX  → RX_PIN of the board
 *
 * SoftwareSerial is reliable up to about 57600 baud; a hardware UART has
 * no such ceiling. 9600 is the default of most of these modules and of
 * this library.
 *************************************************************/

#include <InstantIoT.h>

// The boards whose only UART is the USB one take the module on two
// software pins. GPIO 10 and 11 are wired to the flash chip on most ESP8266
// modules, so the pins are not the same on the two families. On an Uno the
// RX pin must be interrupt-capable: 10 is, many others are not — check the
// SoftwareSerial documentation before moving it.
#if defined(ESP8266)
  #define SOFT_PINS 1
  #define RX_PIN 13   // D7
  #define TX_PIN 15   // D8
#elif defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
  #define SOFT_PINS 1
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

#if defined(SOFT_PINS)
    InstantIoT.begin(SerialLink(RX_PIN, TX_PIN));
#else
    InstantIoT.begin(SerialLink(Serial1));
#endif

    // Every 5 seconds, not every 2: at 9600 baud the line is narrow, and a
    // frame that is still going out when the next one starts is a frame
    // nobody reads.
    timers.every(5000, sendReading);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
