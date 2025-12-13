# ⚡ InstantIoT Library v1.0

Easy IoT communication library for Arduino with mobile app integration.

## Features

* 🔌 **WiFi Access Point**: ESP creates its own WiFi network
* 🧩 **17 Widgets**: Buttons, Sliders, Joystick, D-Pad, LED, Gauge, Chart, and more
* ✨ **Clean DSL**: `ON_PRESS("btn1") { ... }` style macros
* 📊 **Multi-Series Charts**: AdvancedChart and BarChart with multiple data series
* 🔄 **Platform Support**: ESP32, ESP8266
* 📱 **Mobile App Ready**: Compatible with InstantIoT mobile app

## Quick Start

### Example 1: Control (App → Arduino)

Control the built-in LED and a motor with sliders and buttons.

```cpp
#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("MyESP", "12345678");
InstantTimer timers;

// Pin definitions
const int LED_PIN = 2;        // Built-in LED (ESP32)
const int MOTOR_A_PIN = 25;   // Motor driver IN1
const int MOTOR_B_PIN = 26;   // Motor driver IN2

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        digitalWrite(LED_PIN, HIGH);
        instant.led("led1").on();
    }
    ON_RELEASE("btn1") {
        digitalWrite(LED_PIN, LOW);
        instant.led("led1").off();
    }
}

void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
    ON_VALUE_CHANGING("speed") {
        int pwmValue = map(e.value, 0, 100, 0, 255);
        analogWrite(MOTOR_A_PIN, pwmValue);
        instant.gauge("speedGauge").setValue(e.value);
    }
}

void onJoystickEvent(const JoystickEvent& e) {
    ON_JOYSTICK("joy1", x, y) {
        int leftMotor = constrain((y + x) * 255 / 100, -255, 255);
        int rightMotor = constrain((y - x) * 255 / 100, -255, 255);
        analogWrite(MOTOR_A_PIN, abs(leftMotor));
        analogWrite(MOTOR_B_PIN, abs(rightMotor));
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(MOTOR_A_PIN, OUTPUT);
    pinMode(MOTOR_B_PIN, OUTPUT);
    
    if (instant.begin()) {
        Serial.print("WiFi ready! Connect to: MyESP | IP: ");
        Serial.println(instant.getIP());
    }
}

void loop() {
    instant.loop();
    timers.run();
}
```

### Example 2: Display (Arduino → App)

Send simulated sensor data to the app (temperature & humidity like DHT11).

```cpp
#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("MyESP", "12345678");
InstantTimer timers;

float temperature = 22.0;
float humidity = 45.0;

void readSensor() {
    temperature = 20.0 + random(-50, 50) / 10.0;
    humidity = 50.0 + random(-200, 200) / 10.0;
}

void updateDisplay() {
    readSensor();
    
    instant.gauge("temp").setValue(temperature);
    instant.gauge("hum").setValue(humidity);
    
    instant.chart("history").addPoint("temp", temperature);
    instant.chart("history").addPoint("hum", humidity);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "T:%.1f°C H:%.0f%%", temperature, humidity);
    instant.text("status").setText(buf);
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    
    if (instant.begin()) {
        Serial.print("Connect to WiFi: MyESP | IP: ");
        Serial.println(instant.getIP());
    }
    
    timers.every(2000, updateDisplay);
}

void loop() {
    instant.loop();
    timers.run();
}
```

## Supported Platforms

| Platform | Status |
|----------|--------|
| ESP32 | ✅ Supported |
| ESP8266 | ✅ Supported |

## Widgets

### Controllers (App → Arduino)

* SimpleButton, AdvancedButton, EmergencyButton
* HorizontalSlider, VerticalSlider
* Switch, Joystick, DirectionPad, SegmentedSwitch

### Displays (Arduino → App)

* Led, Gauge, Metric
* HorizontalLevel, VerticalLevel
* AdvancedChart (multi-series), BarChart (multi-series)
* Text

## Event Callbacks & DSL Macros

The DSL macros are used inside callback functions:

```cpp
// Callback functions (define these in your sketch)
void onSimpleButtonEvent(const SimpleButtonEvent& e);
void onAdvancedButtonEvent(const AdvancedButtonEvent& e);
void onEmergencyButtonEvent(const EmergencyButtonEvent& e);
void onHorizontalSliderEvent(const HorizontalSliderEvent& e);
void onVerticalSliderEvent(const VerticalSliderEvent& e);
void onSwitchEvent(const SwitchEvent& e);
void onJoystickEvent(const JoystickEvent& e);
void onDirectionPadEvent(const DirectionPadEvent& e);
void onSegmentedSwitchEvent(const SegmentedSwitchEvent& e);
```

### DSL Macros

```cpp
// Buttons (use in onSimpleButtonEvent, onAdvancedButtonEvent)
ON_PRESS("id")           // Button pressed
ON_RELEASE("id")         // Button released
ON_LONG_PRESS("id")      // Long press
ON_TOGGLE("id")          // Button toggled

// Emergency (use in onEmergencyButtonEvent)
ON_TRIGGER("id")         // Emergency triggered

// Sliders (use in onHorizontalSliderEvent, onVerticalSliderEvent)
ON_VALUE_CHANGING("id")  // Slider dragging (real-time)

// Joystick (use in onJoystickEvent)
ON_JOYSTICK("id", x, y)  // Joystick with coordinates

// Switch (use in onSwitchEvent)
ON_TURN_ON("id")         // Switch turned on
ON_TURN_OFF("id")        // Switch turned off

// D-Pad (use in onDirectionPadEvent)
ON_DPAD_UP("id")         // D-Pad up pressed
ON_DPAD_DOWN("id")       // D-Pad down pressed
ON_DPAD_LEFT("id")       // D-Pad left pressed
ON_DPAD_RIGHT("id")      // D-Pad right pressed
ON_DPAD_CENTER("id")     // D-Pad center pressed

// Segmented Switch (use in onSegmentedSwitchEvent)
ON_SELECTION_CHANGED("id") // Selection changed
```

## Connection

1. Upload sketch to ESP32/ESP8266
2. Connect your phone to the WiFi network created by ESP (e.g., "MyESP")
3. Open InstantIoT app and connect to the device

## License

MIT License - Copyright (c) 2025 InstantIoT
