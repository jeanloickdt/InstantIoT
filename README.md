# ⚡ InstantIoT Arduino Library

**Drag-and-drop IoT dashboards for ESP32, ESP8266 and Arduino Uno R4 WiFi.**

Connect your Arduino to the [InstantIoT mobile app](https://instantiot.io) over Wi-Fi — directly, or through a self-hosted InstantIoT Server. Build interactive dashboards in minutes.

---

## Features

- 🔌 **Two active Wi-Fi transports** — Wi-Fi SoftAP (the board hosts its own Wi-Fi) and Wi-Fi client to a self-hosted InstantIoT Server (multi-device, history)
- 🧩 **17 widgets** — Buttons (simple, advanced, emergency), Sliders, Switch, Joystick, D-Pad, Segmented Switch, LED, Gauge, Metric, Bar Chart, Advanced Chart, H/V Level meters, Text
- ✨ **Declarative DSL** — `ISimpleButton("btn1") { WHEN_PRESSED { ... } }` blocks at file scope, no boilerplate event handlers
- ⚡ **Binary protocol** — about 5× smaller and faster than JSON
- 📱 **Self-hosted by design** — connects directly to your phone, or to your own InstantIoT Server for multi-device and history
- 🔍 **Auto platform detection** — include the header, the library handles the rest

> Bluetooth Classic, BLE and SoftwareSerial transports ship in source as
> **preview** — the code is there but the mobile app does not currently
> expose them. See [Preview transports](#preview-transports-not-yet-exposed-by-the-mobile-app) below.

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

| Platform | Wi-Fi SoftAP | Wi-Fi → Server |
|----------|--------------|----------------|
| ESP32 | ✅ | ✅ |
| ESP8266 | ✅ | ✅ |
| Arduino Uno R4 WiFi | ✅ | ✅ |

---

## Active Transports

These transports are end-to-end supported by both the library and the
InstantIoT mobile app.

| Transport | Header | Use case |
|-----------|--------|----------|
| Wi-Fi SoftAP | `InstantIoTWiFiAP.hpp` | The board hosts its own Wi-Fi; the phone joins it. No router. |
| Wi-Fi → InstantIoT Server | `InstantIoTWiFiServer.hpp` | The board connects as a TCP client to a self-hosted InstantIoT Server (multi-device, history). |

---

## Quick Start

### Control (App → Arduino)

The library uses a **declarative DSL**: each widget gets its own `I<Widget>("id") { ... }`
block at file scope, with `WHEN_*` clauses inside. No `void onXxxEvent(...)` boilerplate.

```cpp
#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("InstantIoT", "12345678");

// Declare the button handler at file scope — the library picks it up automatically.
ISimpleButton("btn1") {
    WHEN_PRESSED        { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED       { digitalWrite(LED_BUILTIN, LOW); }
    WHEN_LONG_PRESSED   { Serial.println("btn1 held"); }
    WHEN_TOGGLED(isOn)  { Serial.print("btn1 toggled → "); Serial.println(isOn); }
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

Each control widget has a dedicated `I<Widget>("id") { ... }` block.
Inside, use the `WHEN_*` clauses that apply to that widget type.

| Widget | Block | `WHEN_*` clauses inside the block |
|--------|-------|-----------------------------------|
| SimpleButton | `ISimpleButton("id") { … }` | `WHEN_PRESSED`, `WHEN_RELEASED`, `WHEN_LONG_PRESSED`, `WHEN_TOGGLED(isOn)` |
| AdvancedButton | `IAdvancedButton("id") { … }` | `WHEN_PRESSED`, `WHEN_RELEASED`, `WHEN_LONG_PRESSED`, `WHEN_TOGGLED(isOn)` |
| HorizontalSlider | `IHorizontalSlider("id") { … }` | `WHEN_CHANGING(v)`, `WHEN_CHANGED(v)` |
| VerticalSlider | `IVerticalSlider("id") { … }` | `WHEN_CHANGING(v)`, `WHEN_CHANGED(v)` |
| Switch | `ISwitch("id") { … }` | `WHEN_TURNED_ON`, `WHEN_TURNED_OFF`, `WHEN_TOGGLED(isOn)`, `WHEN_SWITCH_SET(isOn)` |
| Joystick | `IJoystick("id") { … }` | `WHEN_MOVED(x, y)`, `WHEN_RELEASED` |
| DirectionPad | `IDirectionPad("id") { … }` | `WHEN_PAD_PRESSED(btn)`, `WHEN_PAD_RELEASED(btn)`, `WHEN_PAD_LONG_PRESSED(btn)` |
| SegmentedSwitch | `ISegmentedSwitch("id") { … }` | `WHEN_SELECTION_CHANGED(idx)`, `WHEN_SEGMENT_SELECTED(idx)`, `WHEN_SEGMENT_DESELECTED(idx)` |
| EmergencyButton | `IEmergencyButton("id") { … }` | `WHEN_TRIGGERED`, `WHEN_RESET` |

> All blocks are declared at **file scope**, not inside `setup()` or `loop()`.
>
> The legacy `void onEmergencyButtonEvent(const EmergencyButtonEvent& e) { ON_TRIGGER("id") { … } ON_RESET("id") { … } }` handler style still works for backwards compatibility.

### Displays (Arduino → App)

| Widget | Method |
|--------|--------|
| LED | `instant.led("id").setColor(r, g, b).setBrightness(80)` |
| Gauge | `instant.gauge("id").setValue(42.5f)` |
| Metric | `instant.metric("id").setValue(23.1f)` |
| HorizontalLevel | `instant.hLevel("id").update(75.0f, 0.0f, 100.0f)` |
| VerticalLevel | `instant.vLevel("id").update(75.0f, 0.0f, 100.0f)` |
| AdvancedChart | `instant.chart("id").addPoint("series1", value)` |
| BarChart | `instant.barChart("id").setBar(0, 42.0f)` — or `setValues(values, count)` |
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

## Preview transports (not yet exposed by the mobile app)

The library also ships source code for three additional transports that
are **not currently wired into the InstantIoT mobile app**. The headers
are in the source tree, they compile, and the on-the-wire protocol is the
same; but until the app exposes a selector for them, an end-to-end
connection is not possible. They are kept here so the work is not lost
and so contributors can experiment.

| Transport | Header | Extra setup |
|-----------|--------|-------------|
| Bluetooth Classic (ESP32, SPP) | `InstantIoTBluetoothESP32SPP.hpp` | none |
| BLE (ESP32, NimBLE) | `InstantIoTBluetoothBLE.hpp` | install [`NimBLE-Arduino`](https://github.com/h2zero/NimBLE-Arduino) from the Library Manager |
| SoftwareSerial (HC-05 / HC-06) | `InstantIoTSerial.hpp` | external Bluetooth-Serial module wired to the board |

---

## License

MIT License — Copyright (c) 2026 Djoufack Tsobeng Jean Loick — see [`LICENSE`](LICENSE).