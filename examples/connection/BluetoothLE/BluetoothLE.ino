/*************************************************************
 * InstantIoT — Bluetooth Low Energy, no pairing
 *
 * BLE through the Nordic UART Service. The app scans, sees the board by
 * name and connects — nothing to pair in the phone's settings, and
 * nothing to configure on a router. No WiFi, no server, no token.
 *
 * Boards: the whole ESP32 family EXCEPT the S2, which has no Bluetooth
 * radio of any kind. On the original ESP32 both this and
 * `BluetoothClassic.ino` work; on the S3, C3 and C6, only this one.
 *
 *************************************************************
 * ⚠️ THE FIRST LINE IS NOT DECORATION
 *************************************************************
 *
 * `#include <NimBLEDevice.h>` must come BEFORE `<InstantIoT.h>`, and the
 * sketch does not work without it — not "works less well": `BLELink` does
 * not exist at all.
 *
 * The reason is in Arduino's build system, not in this library. It finds
 * libraries by READING the `#include` directives of what it compiles: it
 * preprocesses, meets an include it cannot resolve, goes looking for the
 * library that provides it, adds it to the path, and starts again. A
 * conditional include is never met, so NimBLE never lands on the path, so
 * the condition stays false. The sketch has to break that circle, and
 * this line is how.
 *
 * Install "NimBLE-Arduino" from the Library Manager first.
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

#include <NimBLEDevice.h>
#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;

// Identical to the WiFi sketches, deliberately: the link changed, the
// signals did not.
ISimpleButton(I0) {
    WHEN_PRESSED       { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW);  }
    WHEN_LONG_PRESSED  { InstantIoT.write(I2, "long press"); }
};

void sendTemperature() {
    InstantIoT.write(I1, (float)(analogRead(A0) * 3.3f / 4095.0f));
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // The name the app will see while scanning.
    if (InstantIoT.begin(BLELink("InstantIoT_BLE"))) {
        Serial.println("BLE advertising as InstantIoT_BLE");
    } else {
        Serial.println("BLE failed to start.");
    }

    timers.every(2000, sendTemperature);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
