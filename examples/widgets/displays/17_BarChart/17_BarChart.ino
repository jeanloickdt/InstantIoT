/*************************************************************
 * InstantIoT — Example: BarChart
 *
 * Use case: Multi-sensor environmental dashboard
 *
 * Widget: BarChart  id="env"
 * Board : ESP8266 / ESP32
 *
 * Behavior:
 *   Lit 3 capteurs simulés (température, humidité, pression)
 *   et pousse les 3 valeurs comme un array de bars toutes les
 *   2 secondes. Côté app, l'utilisateur a configuré 3 slots
 *   dans le settings sheet — label + couleur — chaque slot
 *   reçoit la valeur correspondante par index.
 *
 * Slots côté app (configurés une fois) :
 *   #0 → Temp     (rouge, 0..40)
 *   #1 → Humidité (bleu, 0..100)
 *   #2 → Pression (jaune, 980..1040)
 *
 * Protocol (Device → App) :
 *   setValues(values, count)   — push tout l'array
 *   setBar(index, value)       — update d'une seule barre
 *   clear()                    — reset
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("InstantIoT_BarChart", "12345678");
InstantTimer timers;

// ── Capteurs simulés ─────────────────────────────────────────
// Remplacer par de vraies lectures BME280 / DHT22 / etc.
float readTemperature() { return 20.0f + random(-30, 30) / 10.0f; }
float readHumidity()    { return 45.0f + random(-50, 50) / 10.0f; }
float readPressure()    { return 1013.0f + random(-50, 50) / 10.0f; }

// ── BarChart update ──────────────────────────────────────────

void updateBarChart() {
    if (!instant.connected()) return;

    // L'ordre des valeurs doit matcher l'ordre des slots
    // configurés dans l'app (settings sheet du widget).
    float values[3];
    values[0] = readTemperature();
    values[1] = readHumidity();
    values[2] = readPressure();

    instant.barChart("env").setValues(values, 3);
}

// ── Setup / Loop ─────────────────────────────────────────────

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== BarChart Example ===");
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
    Serial.println("Widget: BarChart  id='env'  3 bars (Temp / Humid / Pression)");

    timers.every(2000, updateBarChart);
}

void loop() {
    instant.loop();
    timers.run();
}
