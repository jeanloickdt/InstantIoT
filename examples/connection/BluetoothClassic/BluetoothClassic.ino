/*************************************************************
 * InstantIoT — no network at all, just a pairing
 *
 * Bluetooth Classic (SPP). The phone pairs with the board the way it
 * pairs with a speaker, and the app talks to it over that. No WiFi, no
 * router, no server, no token — and no replay of the last values, since
 * there is nobody who kept them.
 *
 * Look at what is NOT different: the signals, the blocks, `write()`. Only
 * the first line of `begin()` changed. That is the whole idea — a link is
 * a path, and the rest of the sketch never learns which one was taken.
 *
 * Boards: ESP32 ONLY, and only the original one.
 *
 *   The S3, C3 and C6 have BLE and no Bluetooth Classic radio; the S2 has
 *   neither. On those, this sketch does not compile, and the message says
 *   so rather than failing at run time on a bench. `BluetoothLE.ino` is
 *   the sketch for them.
 *
 * Pairing: in the phone's Bluetooth settings, pair with "InstantIoT_BT".
 * Then open the app and connect in direct mode.
 *
 *************************************************************
 * ⚠️ Tools → Partition Scheme → "Huge APP"
 *************************************************************
 *
 * The Bluetooth stack does not fit in the default 1.31 MB application
 * partition. The build says so plainly — *text section exceeds available
 * space in board* — and the fix is in the IDE's Tools menu, not in this
 * sketch: pick a scheme that gives the application more room ("Huge APP",
 * 3 MB, or "Minimal SPIFFS" if you need OTA).
 *
 * `test/boards.sh` passes the same choice as an FQBN option.
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;

// A button in the app, on signal I0 of this board. Same three gestures as
// over WiFi: the link carries the bytes, it does not read them.
ISimpleButton(I0) {
    WHEN_PRESSED       { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW);  }
    WHEN_LONG_PRESSED  { InstantIoT.write(I2, "long press"); }
};

void sendTemperature() {
    // Whatever you actually measure. Here, the chip's own sensor stands in
    // for a real one so the example runs with nothing wired up.
    InstantIoT.write(I1, (float)(analogRead(A0) * 3.3f / 4095.0f));
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // The name the phone will show in its pairing list.
    if (InstantIoT.begin(BluetoothLink("InstantIoT_BT"))) {
        Serial.println("Bluetooth ready — pair with InstantIoT_BT");
    } else {
        Serial.println("Bluetooth failed to start.");
    }

    timers.every(2000, sendTemperature);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
