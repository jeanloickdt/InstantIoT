/*************************************************************
 * InstantIoT Example: Gauge with Timer + Button LED
 * 
 * - Gauge "temp1" se met à jour chaque seconde (simule température)
 * - Button "btn1" toggle la LED intégrée de l'ESP32
 * 
 * Widgets requis dans l'app:
 * - SimpleButton avec id "btn1"
 * - Gauge avec id "temp1"
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

// LED intégrée ESP32
#define LED_PIN 2

// Configuration WiFi
InstantIoTWiFiAP instant("MyESP32", "12345678");

// Timer manager
InstantTimer timers;

// Simulation de température
float temperature = 20.0;
float tempDirection = 0.5;

// État LED
bool ledState = false;

// ════════════════════════════════════════════════════════════
// 🔘 CALLBACK BOUTON
// ════════════════════════════════════════════════════════════

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        Serial.print("LED: ");
        Serial.println(ledState ? "ON" : "OFF");
    }
}

// ════════════════════════════════════════════════════════════
// 🌡️ MISE À JOUR GAUGE
// ════════════════════════════════════════════════════════════

void updateGauge() {
    // Ne rien faire si pas de client
    if (!instant.hasClient()) return;
    
    // Simuler variation de température (oscille entre 15 et 35)
    temperature += tempDirection;
    
    if (temperature >= 35.0) {
        tempDirection = -0.5;
    } else if (temperature <= 15.0) {
        tempDirection = 0.5;
    }
    
    // Envoyer à l'app
    instant.gauge("temp1")
        .setValue(temperature)
        .setRange(0, 50)
        .setUnit("°C");
    
    Serial.print("Gauge: ");
    Serial.println(temperature);
}

// ════════════════════════════════════════════════════════════
// 🚀 SETUP
// ════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   InstantIoT - Gauge + Button Example");
    Serial.println("========================================\n");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    if (instant.begin()) {
        Serial.println("WiFi AP started!");
        Serial.print("SSID: MyESP32 | Pass: 12345678 | IP: ");
        Serial.println(instant.getIP());
    } else {
        Serial.println("FAILED!");
        while(1) delay(1000);
    }
    
    // Timer: mise à jour gauge chaque seconde
    timers.every(1000, updateGauge);
    
    Serial.println("\nWidgets: btn1 (Button), temp1 (Gauge 0-50°C)\n");
}

// ════════════════════════════════════════════════════════════
// 🔄 LOOP
// ════════════════════════════════════════════════════════════

void loop() {
    instant.loop();
    timers.run();
}
