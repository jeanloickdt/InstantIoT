/*************************************************************
 * InstantIoT - Generated Code
 * Dashboard: Yyyy
 * 
 * Widgets:
 * - SimpleButton: "B2"
 * - VerticalSlider: "S1"
 * - Gauge: "G1"
 * - AdvancedChart: "char1"
 * - SimpleButton: "bt"
 * - AdvancedChart: "chart2"
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

// ════════════════════════════════════════════════════════════
// 📡 CONFIGURATION
// ════════════════════════════════════════════════════════════

InstantIoTWiFiAP instant("MyESP32", "12345678");
InstantTimer timers;

// ════════════════════════════════════════════════════════════
// 🎮 CONTROLLER CALLBACKS
// ════════════════════════════════════════════════════════════

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    // SimpleButton: B2
    ON_PRESS("B2") {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    ON_RELEASE("B2") {
         digitalWrite(LED_BUILTIN, LOW);
    }
    ON_LONG_PRESS("B2") {
        // TODO: Add your code
    }
    ON_TOGGLE_STATE("B2", isOn) {
        // TODO: Add your code
    }
    // SimpleButton: bt
    ON_PRESS("bt") {
        // TODO: Add your code
    }
    ON_RELEASE("bt") {
        // TODO: Add your code
    }
    ON_LONG_PRESS("bt") {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    ON_TOGGLE_STATE("bt", isOn) {
          digitalWrite(LED_BUILTIN, isOn);
    }
}

void onVerticalSliderEvent(const VerticalSliderEvent& e) {
    // VerticalSlider: S1

    ON_VALUE_CHANGING("S1") {
        int value = e.value; // 0-100
        // TODO: Add your code
    }
}

// ════════════════════════════════════════════════════════════
// 📊 DISPLAY UPDATES (called by timers)
// ════════════════════════════════════════════════════════════

// Gauge: G1
void updateG1() {
    if (!instant.hasClient()) return;
    
    instant.gauge("G1")
        .setValue(random(15, 70))  // TODO: Replace with your value
        .setRange(0, 70)
        .setUnit("Km/h");  // TODO: Set unit
}

// AdvancedChart: char1
void updateChar1() {
    if (!instant.hasClient()) return;
  
    instant.chart("char1")
        .addPoint(random(0,100));  // TODO: Replace with your value
}

// AdvancedChart: chart2
void updateChart2() {
    if (!instant.hasClient()) return;
    
    instant.chart("chart2")
        .addPoint(random(18,100));  // TODO: Replace with your value
}

// ════════════════════════════════════════════════════════════
// 🚀 SETUP
// ════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // TODO: Initialize your pins
     pinMode(LED_BUILTIN, OUTPUT);
    
    if (instant.begin()) {
        Serial.println("WiFi AP started!");
        Serial.println(instant.getIP());
    } else {
        Serial.println("WiFi FAILED!");
        while(1) delay(1000);
    }
    
    // Display update timers
    timers.every(1000, updateG1);  // Gauge: G1
    timers.every(500, updateChar1);  // AdvancedChart: char1
    timers.every(200, updateChart2);  // AdvancedChart: chart2
}

// ════════════════════════════════════════════════════════════
// 🔄 LOOP
// ════════════════════════════════════════════════════════════

void loop() {
    instant.loop();
    timers.run();
}