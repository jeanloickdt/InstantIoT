/*************************************************************
 * InstantIoT — a thermostat, both directions
 *
 * The board publishes what it measures, and obeys what is written to it.
 * This is the example that best shows what 2.0 changed.
 *
 * The interesting half is the second one. A setpoint is a STATE, not a
 * gesture: the server keeps it and sends it back on connect. Unplug the
 * board, plug it in tomorrow — the `ISignal(I5, …)` block starts again
 * with yesterday's setpoint, with nobody opening the app and nothing
 * written to EEPROM.
 *
 * An `ISimpleButton` would NOT be replayed: nobody pressed. A state is
 * restored, a gesture is not.
 *
 * Signals to declare in the app, on this board:
 *   I0  float   measure   "Temperature"
 *   I5  float   setpoint  "Setpoint"    5 to 30, replay on
 *   I6  bool    setpoint  "Pump"
 *   I7  text    setpoint  "Mode"
 *
 * Replace: WIFI_SSID, WIFI_PASS, DEVICE_TOKEN.
 *
 * Boards: ESP32, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

const char* WIFI_SSID    = "MyWiFi";
const char* WIFI_PASS    = "MyPassword";
const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

#if defined(ESP32)
  #define SENSOR_PIN 34
#else
  #define SENSOR_PIN A0
#endif
#define PUMP_PIN 4

float        setpoint = 19.0f;
InstantTimer timers;

ISignal(I5, float target) {
    setpoint = target;
    Serial.print("Setpoint -> "); Serial.println(setpoint);
};

ISignal(I6, bool running) {
    digitalWrite(PUMP_PIN, running ? HIGH : LOW);
};

ISignal(I7, const char* mode) {
    Serial.print("Mode -> "); Serial.println(mode);
};

// Optional: everything written to this board passes here, including the
// addresses no block claims. That is how you notice a forgotten one.
void onSignalWritten(const SignalEvent& e) {
    Serial.print("signal I"); Serial.print(e.address); Serial.println(" written");
}

void publishMeasurement() {
    float volts = analogRead(SENSOR_PIN) * 3.3f / 4095.0f;
    InstantIoT.write(I0, volts * 100.0f);
}

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);
    InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS), Cloud(DEVICE_TOKEN));
    timers.every(2000, publishMeasurement);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
