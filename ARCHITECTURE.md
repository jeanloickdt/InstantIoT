# InstantIoT Arduino Library — Architecture

> Single-header-per-transport C++ library that connects an Arduino board
> (ESP32 / ESP8266 / Uno R4 WiFi) to the InstantIoT mobile app using a
> compact binary protocol (`iWidgets v1`).
>
> No dynamic memory, no JSON, fully static allocation. Everything fits in a few KB of
> Flash and ~100 B of RAM plus a fixed-size buffer.
>
> Read this end to end (~10 min) before contributing.

---

## 1. Overview — the user's mental model

The library is designed so a maker writes **as little code as possible**:

```cpp
#include <InstantIoTWiFiAP.hpp>            // pick a transport

InstantIoTWiFiAP instant("DeviceName", "12345678");

ISimpleButton("btn1") {                    // declare handlers at file scope
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW); }
}

void setup() { instant.begin(); }
void loop()  { instant.loop(); }
```

That's the whole API surface for a basic dashboard. Display widgets work
through fluent accessors on the instance:

```cpp
instant.gauge("temp").setValue(23.5f);
instant.chart("data").addPoint("default", value);
```

Everything else — protocol framing, CRC, transport selection, callback
dispatch — happens behind the scenes.

---

## 2. The three layers

```
        ┌──────────────────────────────────────────────────┐
        │  user sketch (.ino)                              │
        │  • includes one transport header                 │
        │  • declares I<Widget>("id") { WHEN_* … } blocks  │
        │  • calls instant.<displayWidget>("id").<method>  │
        └────────────────────┬─────────────────────────────┘
                             │
        ┌────────────────────▼─────────────────────────────┐
        │  Façade + Core (transport-agnostic)              │
        │  • InstantIoTWiFiAP / WiFiServer / BLE / …       │
        │  • InstantIoTCoreBase (loop, RX assembly, TX)    │
        │  • BinaryCodec (frame ↔ DecodedMessage)          │
        │  • WidgetRegistry (dispatch event → handler)     │
        └────────────────────┬─────────────────────────────┘
                             │  ITransport (read / write / poll)
        ┌────────────────────▼─────────────────────────────┐
        │  Transport implementations                       │
        │  • WiFi SoftAP / WiFi Server / BLE / BT SPP /    │
        │    SoftwareSerial                                │
        └──────────────────────────────────────────────────┘
```

| Layer | Owns | Knows about |
|---|---|---|
| User sketch | App-specific logic | The widgets it uses |
| Façade + Core | Protocol, dispatch, widget structs | An `ITransport` (abstract) |
| Transport | Raw bytes over the wire | Nothing protocol-specific |

This is what lets the same widget code run over Wi-Fi today and over BLE
tomorrow without a single user-facing change.

---

## 3. Module tree — `src/`

```
src/
├─ InstantIoT*.{hpp,h}                façade per transport (the only header
│   │                                  the user includes)
│   ├─ InstantIoTWiFiAP.hpp             board hosts its own Wi-Fi (SoftAP)
│   ├─ InstantIoTWiFiServer.hpp         board → TCP client of InstantIoT Server
│   ├─ InstantIoTBluetoothESP32SPP.hpp  Bluetooth Classic, ESP32 only
│   ├─ InstantIoTBluetoothBLE.hpp       BLE GATT, ESP32 (NimBLE)
│   └─ InstantIoTSerial.hpp             HC-05 / HC-06 over SoftwareSerial
│
├─ InstantIoTConfig.h                   compile-time flags + buffer sizes
│
├─ core/                                ★ protocol & dispatch — transport-agnostic
│   ├─ Codec.h                          shared types: DecodedMessage, Param
│   ├─ BinaryCodec.hpp                  encode/decode iWidgets v1 frames
│   ├─ Transport.h                      ITransport interface
│   ├─ MessageSender.h                  IMessageSender interface (for widgets)
│   ├─ Registry.hpp / Registry.cpp      dispatch event → user callback
│   ├─ InstantIoTCore.hpp               main loop: RX assembly + TX + heartbeat
│   ├─ InstantIoTMessage.hpp            typed event structs (SimpleButtonEvent…)
│   └─ InstantIoTDeviceConfig.hpp       device identity + broker info
│
├─ widgets/
│   ├─ WidgetBase.hpp                   base class for display widgets
│   ├─ WidgetIncludes.hpp               aggregator gated by INSTANTIOT_WIDGETS_*
│   └─ displays/                        Arduino → App (no controls/ dir on
│       │                               purpose: control events are received,
│       │                               never emitted)
│       ├─ Gauge.hpp, Metric.hpp, Led.hpp, Text.hpp
│       ├─ HorizontalLevel.hpp, VerticalLevel.hpp
│       └─ AdvancedChart.hpp, BarChart.hpp
│
├─ transport/                           concrete ITransport implementations
│   ├─ serial/InstantSoftwareSerial.hpp
│   ├─ bluetooth/{BT_ESP32.hpp, BT_ESP32_BLE.hpp}
│   └─ wifi/{SoftAP_ESP32.hpp, SoftAP_ESP8266.hpp,
│            SoftAP_R4.hpp, WiFiServerClient_ESP32.hpp}
│
└─ utils/
    ├─ InstantIoTMacros.hpp             legacy DSL: void onXxxEvent + ON_* macros
    ├─ InstantIoTWhen.hpp               modern DSL: I<Widget>("id"){ WHEN_* … }
    ├─ InstantIoTDebug.hpp              IIOT_LOG (compiled out if !INSTANTIOT_DEBUG)
    ├─ InstantIoTTimer.hpp              non-blocking timing helpers
    └─ InstantIoTColor.hpp              rgb / hex color helpers
```

---

## 4. Two DSLs for the same problem

The library historically exposed callbacks. A newer, cleaner style has
replaced them for most controls.

### Modern (recommended) — declarative, file-scope blocks

```cpp
ISimpleButton("btn1") {
    WHEN_PRESSED        { … }
    WHEN_RELEASED       { … }
    WHEN_LONG_PRESSED   { … }
    WHEN_TOGGLED(isOn)  { … }
}
```

`InstantIoTWhen.hpp` expands `I<Widget>(id) { … }` into:
- a static function definition that handles the events for that widget
  (the user's `{ … }` becomes the function body)
- a `WidgetRegistrar<EventT>` instance whose constructor adds itself to a
  global intrusive linked list **before `setup()` runs**

The blocks must live at **file scope**, not inside `setup()` or `loop()`.

Available blocks: `ISimpleButton`, `IAdvancedButton`, `IEmergencyButton`,
`IHorizontalSlider`, `IVerticalSlider`, `ISwitch`, `IJoystick`,
`IDirectionPad`, `ISegmentedSwitch`.

### Legacy — global event handlers

```cpp
void onSimpleButtonEvent(const SimpleButtonEvent& e) {
    ON_PRESS("btn1") { … }
}
```

These free functions are declared `__attribute__((weak))` with an empty
default in `Registry.cpp`. When the user defines one in their sketch,
the linker keeps the user's version.

Both DSLs coexist: `WidgetRegistry::dispatch()` calls the weak callback
**first** and then walks the handler list. You can mix both styles in the
same sketch — for example use modern `I<Widget>` blocks for new widgets
while keeping an existing `void onSimpleButtonEvent(…)` from older code.

---

## 5. The Registry — how a button press reaches your callback

```
Frame on the wire
  │
  ▼
InstantIoTCoreBase::loop()
  ├─ _transport.poll()
  ├─ readLoop()                  read up to 64 bytes → _rxBuffer (4 KB)
  └─ extractFrames()              scan for [AA 01 LEN(2B) … CRC]
        │
        ▼
  processFrame(buffer, len)
        │
        ▼
  BinaryCodec::decode()
   • validate magic + version + CRC8
   • parse DEV_COUNT / DEV / WID_LEN / WID / TYPE / EVENT
   • decodePayload(typeCode, eventCode, …) fills msg.params[]
   • result: a DecodedMessage
        │
        ▼
  WidgetRegistry::dispatch(typeCode, widgetId, eventCode, msg)
   • switch(typeCode) → build a typed event struct
       (SimpleButtonEvent, JoystickEvent, …)
   • call the weak global callback (if user defined one)
   • dispatchToHandlers(e): walk handlerListHead<EventT>(),
       call each WidgetHandler whose widgetId matches via strcmp
```

`handlerListHead<EventT>()` is a `template<typename E> static WidgetHandler*&`
— one linked-list root per event struct type. Each `WidgetRegistrar` is a
~12 B node allocated at file scope; nothing on the heap.

---

## 6. The other direction — sending a display update

```
User code
  │
  ▼
instant.gauge("temp").setValue(23.5f)
  │
  ▼
GaugeWidget::setValue(float v)
   • writes the float LE into a local 4-byte buffer
   • calls sendBinary(EV_SETVALUE, buf, 4)
  │
  ▼
WidgetBase::sendBinary(eventCode, payload, len)
   • delegates to IMessageSender (the core)
  │
  ▼
InstantIoTCoreBase::sendBinary(widgetId, typeCode, eventCode, payload, len)
   • BinaryCodec::encode(_txBuffer, deviceId, widgetId, typeCode,
                          eventCode, payload, len)
   • _transport.write(_txBuffer, len)
   • bytes go out the wire
```

Display widget classes (`GaugeWidget`, `LedWidget`, `BarChartWidget`, …)
all inherit `DisplayWidget` which inherits `WidgetBase`. The base owns
the widget id (fixed-size `char[]`) and the sender reference.

---

## 7. Transports — the `ITransport` contract

```cpp
class ITransport {
public:
    virtual bool begin() = 0;          // open the connection
    virtual void poll() = 0;           // service the underlying stack
    virtual int  available() = 0;
    virtual int  read(uint8_t* buf, size_t n) = 0;
    virtual int  write(const uint8_t* buf, size_t len) = 0;
    virtual bool connected() = 0;
};
```

Every façade (`InstantIoTWiFiAP`, `InstantIoTBluetoothBLE`, …) creates
the right `ITransport` implementation for the board and wires it into
`InstantIoTCoreBase`. The core never sees Wi-Fi or BLE directly — only
the abstract interface — which is what makes the library trivial to
extend with new physical media.

Currently shipped:

- `SoftAP_ESP32` / `SoftAP_ESP8266` / `SoftAP_R4` — board hosts its own
  Wi-Fi access point, phone joins it
- `WiFiServerClient_ESP32` — board connects as a TCP client to a
  self-hosted InstantIoT Server
- `BT_ESP32` — Bluetooth Classic SPP (preview, app not exposing it)
- `BT_ESP32_BLE` — BLE GATT via NimBLE (preview)
- `InstantSoftwareSerial` — HC-05 / HC-06 (preview)

---

## 8. Memory model

The library is **statically allocated end to end** — predictable for AVR
and ESP8266 RAM budgets.

| Buffer | Size | Purpose |
|---|---|---|
| `_rxBuffer[INSTANT_RX_BUFFER_SIZE]` | 4 KB default | Stream reassembly, frame extraction |
| `_txBuffer[INSTANT_TX_BUFFER_SIZE]` | 512 B default | Encoded outgoing frame |
| `body[256]` (local in `BinaryCodec::encode`) | 256 B | Stack scratchpad while assembling a frame |

| Per-widget allocation | Where |
|---|---|
| `_gauges[INSTANTIOT_MAX_WIDGETS]` (and similar arrays per widget kind) | Member array in the core — fixed size, typically 16 |
| `WidgetHandler` nodes from `I<Widget>{}` blocks | One ~12 B static node per block, linked into a global list at startup |

`INSTANTIOT_MAX_WIDGETS` (default 16) caps the number of unique widget
ids the device can address per kind. Beyond that, new ids are silently
ignored — keeps Flash and RAM predictable.

No `String` Arduino class, no `std::string` — only fixed-size `char[]`
of `INSTANTIOT_MAX_WIDGET_ID_LENGTH` (32 by default).

---

## 9. Compile-time configuration — `InstantIoTConfig.h`

Every widget can be turned off at compile time to save Flash and RAM:

```cpp
#define INSTANTIOT_WIDGETS_GAUGE          1
#define INSTANTIOT_WIDGETS_LED            1
#define INSTANTIOT_WIDGETS_ADVANCEDCHART  1
#define INSTANTIOT_WIDGETS_SEGSWITCH      0   // disabled
…
```

A flag set to `0` removes:
- the include of the widget header
- the array + count member in the core
- the public accessor (`gauge("id")`, etc.)
- the corresponding case in `BinaryCodec::decodePayload`'s `switch(typeCode)`
- the destructor cleanup for that widget kind

Useful on Uno / Mega / Nano targets where every kilobyte counts.

Other knobs:

```cpp
#define INSTANTIOT_DEBUG                  0  // 1 → IIOT_LOG to Serial
#define INSTANTIOT_MAX_WIDGETS            16
#define INSTANTIOT_MAX_WIDGET_ID_LENGTH   32
#define INSTANT_RX_BUFFER_SIZE            4096
#define INSTANT_TX_BUFFER_SIZE            512
#define INSTANT_AP_PORT                   8888
```

---

## 10. Heartbeat (TCP server mode only)

`InstantIoTWiFiServer` exposes `setHeartbeat(uint32_t intervalMs)` (default
5000 ms). The library periodically emits a tiny frame:

```
TYPE = 0xFE (HEARTBEAT)   WID_LEN = 0   EVENT = 0   PAYLOAD = {}
```

The server resets the socket read timeout on every received byte, and
flags the device offline after `intervalMs × 2.5` of silence. Heartbeats
are **never relayed** to mobile apps — pure liveness plumbing.

---

## 11. Onboarding — where to start

Read these files, in this order:

1. `src/InstantIoTWiFiAP.hpp` — the most common façade, ~50 lines
2. `src/core/InstantIoTCore.hpp` — the main loop and RX assembly
3. `src/core/BinaryCodec.hpp` — the wire protocol, byte by byte
4. `src/core/Registry.hpp` — dispatch + the `WHEN_*` plumbing
5. `src/utils/InstantIoTWhen.hpp` — the user-facing declarative macros
6. `src/widgets/WidgetBase.hpp` + one display widget (e.g. `Gauge.hpp`)
7. `src/transport/wifi/SoftAP_ESP32.hpp` — a reference `ITransport` impl

### Adding a new display widget

1. Create `src/widgets/displays/MyWidget.hpp` with a `MyWidgetWidget`
   class extending `DisplayWidget`. Expose a fluent API
   (`setValue`, `setColor`, …) that internally calls `sendBinary(…)`.
2. Pick an unused `TYPE_*` code in `BinaryCodec.hpp`.
3. Add the include to `WidgetIncludes.hpp` behind a new
   `INSTANTIOT_WIDGETS_MYWIDGET` flag.
4. Add the array + accessor in `InstantIoTCore.hpp` (look at `Gauge` for
   the template).
5. On the Android side, add the matching TYPE and EVENT codes to
   `BinaryTypeRegistry` / `BinaryEventRegistry`, and the
   `decodePayload` / `encodePayload` branches.

### Adding a new control widget

1. Define an event struct in `InstantIoTMessage.hpp`
   (e.g. `MyButtonEvent`).
2. Add the `case` in `WidgetRegistry::dispatch` that builds the struct
   from the `DecodedMessage` and calls the weak callback +
   `dispatchToHandlers`.
3. Add an `IMyButton(id)` macro in `InstantIoTWhen.hpp` mirroring
   `ISimpleButton`, plus the `WHEN_*` clauses it supports.
4. Update the Android encoder/decoder symmetrically.

### Adding a transport

1. Implement `ITransport` in `src/transport/<medium>/<Name>.hpp`.
2. Create a façade header `src/InstantIoT<Name>.hpp` that instantiates
   the transport and `InstantIoTCoreBase`.
3. Document it in `README.md`.

---

**In one sentence:** the library is a small, transport-agnostic engine
that turns user-friendly declarative macros (`ISimpleButton` /
`WHEN_PRESSED`) into compact binary frames over any `ITransport`, with
zero dynamic memory.
