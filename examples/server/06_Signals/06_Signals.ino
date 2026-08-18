/*************************************************************
 * InstantIoT — Example: signals, both directions
 *
 * A thermostat, complete: the board reports the temperature it
 * measures, and obeys the setpoint the app writes back.
 *
 * The point of the example is the second half. A setpoint is a
 * STATE, not a gesture — so the server stores it and replays it
 * on connect. Unplug the board, plug it back in tomorrow: the
 * ISignal(I5) block runs again with the value that was set, with
 * nobody's phone open and nothing saved in EEPROM.
 *
 * Signals to declare in the app, on THIS board:
 *   I0  float   measure   "Température"
 *   I5  float   setpoint  "Consigne"      min 5, max 30
 *   I6  bool    setpoint  "Pompe"
 *   I7  string  setpoint  "Mode"
 *
 * Board : ESP32
 *************************************************************/

#include <InstantIoTWiFiServer.hpp>

const char* WIFI_SSID      = "YOUR_WIFI_SSID";
const char* WIFI_PASS      = "YOUR_WIFI_PASSWORD";
const char* SERVER_IP      = "192.168.1.42";
const uint16_t SERVER_PORT = 9001;
const char* DEVICE_TOKEN   = "PASTE_YOUR_DEVICE_TOKEN";

#define SENSOR_PIN 34
#define PUMP_PIN    4

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);

// ── Ce que le serveur écrit, la carte obéit ─────────────────

float setpoint = 19.0;          // survives nothing here — the server does that

ISignal(I5) {
    WHEN_WRITTEN(float target) {
        setpoint = target;
        Serial.print("Consigne → ");
        Serial.println(setpoint);
    }
};

ISignal(I6) {
    WHEN_WRITTEN(bool on) {
        digitalWrite(PUMP_PIN, on ? HIGH : LOW);
    }
};

ISignal(I7) {
    WHEN_WRITTEN(const char* mode) {
        Serial.print("Mode → ");
        Serial.println(mode);
    }
};

// Optional: everything, including addresses no block above claims.
void onSignalWritten(const SignalEvent& e) {
    Serial.print("[signal] I");
    Serial.println(e.address);
}

// ── Ce que la carte mesure, le serveur reçoit ───────────────

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);

    instant.connectWiFi(WIFI_SSID, WIFI_PASS);
    instant.begin();
}

void loop() {
    instant.loop();

    float celsius = analogRead(SENSOR_PIN) * (3.3 / 4095.0) * 100.0;

    // Safe to call on every pass: the library applies the same ceiling the
    // server's fuse enforces, so this cannot get the board disconnected.
    instant.write(I0, celsius);

    // The board decides what to do with the setpoint it was given.
    digitalWrite(PUMP_PIN, celsius < setpoint ? HIGH : LOW);
}
