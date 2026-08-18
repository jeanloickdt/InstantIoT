/*************************************************************
 * InstantIoT — Example: signals, both directions (Cloud / TLS)
 *
 * A thermostat, complete: the board reports the temperature it
 * measures, and obeys the setpoint written from the app.
 *
 * The point of the example is the second half. A setpoint is a
 * STATE, not a gesture — so the server stores it and replays it
 * on connect. Unplug the board, plug it back in tomorrow: the
 * ISignal(I5, …) block runs again with the value that was set,
 * with nobody's phone open and nothing saved in EEPROM.
 *
 * Signals to declare in the app, on THIS board:
 *   I0  float   measure   "Température"
 *   I5  float   setpoint  "Consigne"      min 5, max 30
 *   I6  bool    setpoint  "Pompe"
 *   I7  string  setpoint  "Mode"
 *
 * Before flashing, replace:
 *   WIFI_SSID, WIFI_PASS   → your router
 *   SERVER_HOST            → your cloud hostname (NOT a raw IP:
 *                            the hostname is needed for SNI)
 *   DEVICE_TOKEN           → token from the cloud panel
 *
 * Self-hosted instead? Two lines change and nothing else:
 *   #include <InstantIoTWiFiServer.hpp>
 *   InstantIoTWiFiServer instant(SERVER_IP, 9001, DEVICE_TOKEN);
 *
 * Boards: ESP32, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoTWiFiServerSecure.hpp>

const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";
const char* SERVER_HOST  = "instantiot.cloud";   // hostname, pas une IP
const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

#define SENSOR_PIN 34
#define PUMP_PIN    4

// Port 9443 par défaut, identité du serveur vérifiée contre les
// racines Let's Encrypt embarquées.
InstantIoTWiFiServerSecure instant(SERVER_HOST, DEVICE_TOKEN);

// ── Ce que le serveur écrit, la carte obéit ─────────────────
//
// No WHEN_ guard: a signal has one thing that can happen to it.
// You write the type of what you receive, because the board is
// the only place that knows what the address holds.

float setpoint = 19.0;

ISignal(I5, float target) {
    setpoint = target;
    Serial.print("Consigne → ");
    Serial.println(setpoint);
};

ISignal(I6, bool on) {
    digitalWrite(PUMP_PIN, on ? HIGH : LOW);
};

ISignal(I7, const char* mode) {
    Serial.print("Mode → ");
    Serial.println(mode);
};

// Optional: everything, including addresses no block above claims.
void onSignalWritten(const SignalEvent& e) {
    Serial.print("[signal] I");
    Serial.println(e.address);
}

// ── Ce que la carte mesure, le serveur reçoit ───────────────

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] connexion impossible — voir le log série");
        // loop() keeps retrying (auto-reconnect with backoff)
    }
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
