# ⚡ InstantIoT Arduino Library

**Drag-and-drop IoT dashboards for ESP32, ESP8266 and Arduino Uno R4 WiFi.**

Connect your Arduino to the [InstantIoT mobile app](https://instantiot.io) via WiFi or Bluetooth. Build interactive dashboards in minutes.

---

## Features

- 🔌 **Multiple transports** — WiFi SoftAP, Bluetooth Classic (ESP32), BLE (ESP32), SoftwareSerial
- 🧩 **16 widgets** — Buttons, Sliders, Joystick, D-Pad, LED, Gauge, Metric, Chart, and more
- ✨ **Clean DSL** — `ON_PRESS("btn1") { ... }` style macros
- ⚡ **Binary protocol** — 5x smaller and faster than JSON
- 📱 **Local-first** — direct device-to-app communication, no subscription
- 🔍 **Auto platform detection** — include the header, the library handles the rest

---

## Installation

### Arduino Library Manager (recommended)
1. Open Arduino IDE → **Sketch → Include Library → Manage Libraries**
2. Search for **InstantIoT**
3. Click **Install**

### Manual
Download the latest release and extract to your Arduino `libraries/` folder.

---

## Supported Platforms

| Platform | WiFi | Bluetooth Classic | BLE | Serial |
|----------|------|-------------------|-----|--------|
| ESP32 | ✅ | ✅ | ✅ experimental | — |
| ESP8266 | ✅ | — | — | ✅ |
| Arduino Uno R4 WiFi | ✅ | — | — | — |
| Arduino Uno / Mega / Nano | — | — | — | ✅ experimental |

> Serial transport requires an external HC-05 or HC-06 Bluetooth module.

---

## Supported Transports

| Transport | Header |
|-----------|--------|
| WiFi SoftAP | `InstantIoTWiFiAP.hpp` |
| Bluetooth Classic (ESP32) | `InstantIoTBluetoothESP32SPP.hpp` |
| BLE NimBLE (ESP32) | `InstantIoTBluetoothBLE.hpp` |
| SoftwareSerial | `InstantIoTSerial.hpp` |

---

## Quick Start

### Control (App → Arduino)

```cpp
#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("InstantIoT", "12345678");

void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    ON_RELEASE("btn1") {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    instant.begin();
    Serial.print("IP: "); Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
```

### Display (Arduino → App)

```cpp
#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("InstantIoT", "12345678");
InstantTimer timers;

void sendSensorData() {
    if (!instant.connected()) return;
    float temperature = 22.5f;
    instant.gauge("temp").setValue(temperature);
    instant.metric("metric1").setValue(temperature);
    instant.chart("chart1").addPoint("default", temperature);
}

void setup() {
    Serial.begin(115200);
    instant.begin();
    timers.every(1000, sendSensorData);
    Serial.print("IP: "); Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
```

---

## Widgets

### Controls (App → Arduino)

| Widget | Callback | Key Macros |
|--------|----------|------------|
| SimpleButton | `onSimpleButtonEvent` | `ON_PRESS`, `ON_RELEASE`, `ON_LONG_PRESS` |
| AdvancedButton | `onAdvancedButtonEvent` | `ON_PRESS`, `ON_RELEASE`, `ON_LONG_PRESS` |
| EmergencyButton | `onEmergencyButtonEvent` | `ON_TRIGGER`, `ON_RESET` |
| HorizontalSlider | `onHorizontalSliderEvent` | `ON_VALUE_CHANGING`, `ON_VALUE_CHANGED` |
| VerticalSlider | `onVerticalSliderEvent` | `ON_VALUE_CHANGING`, `ON_VALUE_CHANGED` |
| Switch | `onSwitchEvent` | `ON_TURN_ON`, `ON_TURN_OFF` |
| Joystick | `onJoystickEvent` | `ON_JOYSTICK(id, x, y)` |
| DirectionPad | `onDirectionPadEvent` | `ON_DPAD_UP`, `ON_DPAD_DOWN`, `ON_DPAD_LEFT`, `ON_DPAD_RIGHT` |
| SegmentedSwitch | `onSegmentedSwitchEvent` | `ON_SELECTION_CHANGED` |

### Displays (Arduino → App)

| Widget | Method |
|--------|--------|
| LED | `instant.led("id").setColor(r, g, b).setBrightness(80)` |
| Gauge | `instant.gauge("id").setValue(42.5f)` |
| Metric | `instant.metric("id").setValue(23.1f)` |
| HorizontalLevel | `instant.hLevel("id").update(75.0f, 0.0f, 100.0f)` |
| VerticalLevel | `instant.vLevel("id").update(75.0f, 0.0f, 100.0f)` |
| AdvancedChart | `instant.chart("id").addPoint("series1", value)` |
| Text | `instant.text("id").setText("Hello!")` |

---

## Examples

All examples are in **File → Examples → InstantIoT**:

| # | Example | Widget |
|---|---------|--------|
| 01 | SimpleButton | SimpleButton |
| 02 | Switch | Switch |
| 03 | LED | LED indicator |
| 04 | HorizontalSlider | HorizontalSlider |
| 05 | Joystick | Joystick |
| 06 | DirectionPad | DirectionPad |
| 07 | SegmentedSwitch | SegmentedSwitch |
| 08 | Gauge | Gauge x4 |
| 09 | Metric | Metric |
| 10 | HorizontalLevel | HorizontalLevel + HC-SR04 |
| 11 | VerticalLevel | VerticalLevel + HC-SR04 |
| 12 | Text | Text |
| 13 | AdvancedChart | AdvancedChart + HC-SR04 |
| 14 | Switch | Switch + relay |
| 15 | LED indicator | LED + temperature |

> All examples work on ESP32, ESP8266 and Arduino Uno R4 WiFi. The library auto-detects your board.

---

## Memory Optimization

Disable unused widgets to save Flash and RAM:

```cpp
#define INSTANTIOT_WIDGETS_ADVANCEDCHART 0
#define INSTANTIOT_WIDGETS_SEGSWITCH     0
#include <InstantIoTWiFiAP.hpp>
```

---

## Debug Mode

```cpp
#define INSTANTIOT_DEBUG 1
#include <InstantIoTWiFiAP.hpp>
```

---

## Links

- 🌐 [instantiot.io](https://instantiot.io)
- 📱 [InstantIoT on Google Play](https://play.google.com/store/apps/details?id=com.jeanloickdt.instantiot)
- 💬 [Community & Support](mailto:bonjour@jeanloickdt.com)

---

## License

MIT License — Copyright (c) 2025 JeanLoick DT