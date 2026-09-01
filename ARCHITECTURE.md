# InstantIoT Arduino Library — Architecture

> A header-only C++ library that connects an Arduino board (ESP32 / ESP8266 /
> Uno R4 WiFi) to the InstantIoT mobile app over a compact binary protocol.
>
> No dynamic memory, no JSON, fully static allocation.
>
> Read this end to end (~10 min) before contributing.

*(This file is in English because the repository is public and the README is.
The source comments are in French — that is deliberate and not an
inconsistency to "fix": the comments argue about design decisions, and they
were written in the language those decisions were made in.)*

---

## 1. The one idea

**The board writes a value at an address. The app decides what it looks
like.**

Everything below follows from that sentence. There are no widget objects on
the board any more, no widget ids on the wire, and no `gauge("temp")`. A
sketch writes `InstantIoT.write(I0, 23.4f)`, and whether that becomes a
gauge, a chart or a number is a choice made in the app — changed without
reflashing.

```cpp
#include <InstantIoT.h>

ISimpleButton(I0) {
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW);  }
};

void setup() {
    InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
}

void loop() {
    InstantIoT.loop();
    InstantIoT.write(I1, analogRead(A0) * 3.3 / 4095.0);
}
```

That is the whole public surface: one header, one object, one `begin`. No
`using namespace` — `InstantIoT.h` ends with named `using` declarations that
put the sketch-facing names (`AccessPoint`, `WiFiLink`, `Cloud`, `MyServer`,
`DPadButton`, …) at global scope, and leave the internals inside `iiot`.

Timing belongs to `InstantTimer`, which comes with the same header:

```cpp
InstantTimer timers;
void publish() { InstantIoT.write(I0, readSensor()); }

void setup() { …; timers.every(1000, publish); }
void loop()  { InstantIoT.loop(); timers.run(); }
```

`delay()` would be wrong here — it stops reading incoming frames too.

---

## 2. The three layers

```
        ┌──────────────────────────────────────────────────┐
        │  user sketch (.ino)                              │
        │  • #include <InstantIoT.h>                       │
        │  • declares I<Widget>(Ix) { WHEN_* … } blocks     │
        │  • calls InstantIoT.write(Ix, value)             │
        └────────────────────┬─────────────────────────────┘
                             │
        ┌────────────────────▼─────────────────────────────┐
        │  Facade + Core (transport-agnostic)              │
        │  • iiot::Facade — the singleton, holds the core  │
        │  • InstantIoTCoreBase (loop, RX assembly, TX)    │
        │  • BinaryCodec (frame ↔ signal)                  │
        │  • SignalEvents (dispatch address → block)       │
        └────────────────────┬─────────────────────────────┘
                             │  ITransport (begin/poll/read/write)
        ┌────────────────────▼─────────────────────────────┐
        │  Transports                                      │
        │  • SoftAP_* / TcpClient_* / TlsClient_*          │
        │  • Bluetooth_ESP32 / BLE_ESP32 / SoftSerial      │
        └──────────────────────────────────────────────────┘
```

| Layer | Owns | Knows about |
|---|---|---|
| Sketch | Application logic | The addresses it uses |
| Facade + Core | Protocol, dispatch, rate ceiling | An `ITransport` (abstract) |
| Transport | Raw bytes over the wire | Nothing protocol-specific |

---

## 3. Module tree — `src/`

```
src/
├─ InstantIoT.h          ★ the only header a sketch includes
├─ InstantIoT.cpp          the singleton itself, and one weak default
├─ Links.hpp               how the board reaches the network
├─ Destinations.hpp        who the board talks to
├─ InstantIoTConfig.h      buffer sizes, platform detection, debug flag
│
├─ core/
│   ├─ Transport.h              ITransport interface
│   ├─ MessageSender.h          IMessageSender interface
│   ├─ Codec.h                  shared types (DecodedMessage, Param)
│   ├─ BinaryCodec.hpp          encode/decode — frames and signals
│   ├─ InstantIoTCore.hpp       main loop: RX assembly, TX, heartbeat
│   ├─ InstantIoTSignals.hpp    SignalRef, the I0..I255 constants
│   ├─ SignalEvents.hpp         SignalValue, the handler list, dispatch
│   ├─ SignalToWidget.hpp       gesture/position/pad decoding
│   ├─ InstantIoTMessage.hpp    typed event structs (SimpleButtonEvent…)
│   └─ InstantIoTDeviceConfig.hpp
│
├─ transport/
│   ├─ wifi/SoftAP_{ESP32,ESP8266,R4}.hpp     board hosts its own WiFi
│   ├─ wifi/TcpClient_{ESP32,R4}.hpp          plain TCP to a server
│   ├─ wifi/TlsClient_{ESP32,R4}.hpp          TLS to a server
│   ├─ bluetooth/{Bluetooth_ESP32,BLE_ESP32}.hpp
│   └─ serial/SoftSerial.hpp
│
├─ certs/InstantIoT_LE_Roots.h    Let's Encrypt roots, for TLS
│
└─ utils/
    ├─ InstantIoTWhen.hpp     the DSL: I<Widget>(Ix) { WHEN_* … }
    ├─ InstantIoTDebug.hpp    IIOT_LOG (compiled out unless INSTANTIOT_DEBUG)
    ├─ InstantIoTTimer.hpp    non-blocking timing helpers
    └─ InstantIoTColor.hpp    rgb / hex helpers
```

The transports keep the chip in their name on purpose. `SoftAP_ESP32` and
`SoftAP_ESP8266` are genuinely different implementations, and a common name
would suggest reading one teaches you the other.

`TcpClient` / `TlsClient` used to be called `WiFiServerClient` and
`WiFiServerClientSecure` — names that said *server* about a client. What
separates the two is the encryption, not the wire, which is where an Ethernet
implementation will slot in.

---

## 4. The singleton

`InstantIoT` is one global object of type `iiot::Facade`, defined in
`InstantIoT.cpp`. It holds a pointer to the core, and the core does the work.

Two properties are load-bearing, and both are tested in
`test/host/test_singleton.cpp`:

**It is callable from anywhere.** Any function, any `ISignal` block, any
`WHEN_` clause. Answering a gesture with a write is the normal case, and the
read and write paths do not share a buffer (`_rxBuffer` on one side,
`_txBuffer` on the other). The one place it must *not* be called from is an
ISR — writing to a socket from an interrupt is unsafe with any library.

**A call before `begin()` does nothing and says so once.** The facade has no
constructor, so its fields are constant-initialised: they are valid even
while another translation unit's globals are still being constructed. A
plain global would not guarantee that, and the failure mode is a board that
reboots at startup for reasons nobody can see.

The namespace was renamed `InstantIoT` → `iiot` to free the name. In C++ an
object and a namespace cannot share one, and the sketch is the side that
needs the readable name.

---

## 5. Links and destinations

A **link** is the path, a **destination** is the far end. They do not know
about each other, which is what will let an `EthernetLink()` appear without
any destination changing.

```cpp
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
InstantIoT.begin(AccessPoint("MyBoard", "12345678"));   // no far end to name
```

Both are **descriptions**, not live objects: they do not survive the
statement. The facade builds the transport from them and keeps it in a
function-local `static`.

| | ESP32 | Uno R4 WiFi | ESP8266 | AVR |
|---|---|---|---|---|
| `AccessPoint` | ✓ | ✓ | ✓ | — |
| `WiFiLink` | ✓ | ✓ | — | — |
| `Cloud` / `MyServer` plain | ✓ | ✓ | — | — |
| `Cloud` / `MyServer` TLS | ✓ | ✓ | — | — |
| `BluetoothLink` | ✓ (BR/EDR only) | — | — | — |
| `BLELink` | ✓ (see below) | — | — | — |
| `SerialLink` | — | — | ✓ | ✓ |

**SAMD (Nano 33 IoT, MKR)**: the core and the DSL compile — 5 % of Flash on a
Nano 33 IoT — but no transport exists, so no link does. Adding the family is
one `ITransport` over WiFiNINA plus one branch in `Links.hpp`; nothing in the
protocol or the DSL stands in the way. (It used to: a single `dtostrf` call
in `BinaryCodec` made the whole library uncompilable on SAMD, because that
function comes from avr-libc and the SAMD core does not have it.)

AVR is not in `library.properties`'s `architectures` and the app does not
expose the serial mode, but `SerialLink` does build on an Uno — 20 % of Flash
and 79 % of RAM, which leaves little room for a sketch.

A combination the board cannot do fails to compile, and the message says what
to write instead rather than talking about templates. Links that do not exist
on a platform still have a shell declaration, so the error is the real reason
and not `did you mean 'WiFiClient'?`.

### The `__has_include` trap

Arduino's build system discovers libraries by **reading `#include`
directives**: it preprocesses, sees an include it cannot resolve, finds the
library that provides it, adds it to the search path, and tries again.

`__has_include` never fails to resolve — it quietly returns 0 — so the
library is never added to the path, so it returns 0. The condition is
self-fulfilling.

That is why the platform guards for `SoftSerial` are plain
`#if defined(ARDUINO_ARCH_AVR) || …` and not `__has_include`. With
`__has_include`, `SerialLink` was unreachable on **every** board, an Uno
included, and nothing said so.

`BLELink` keeps `__has_include`, because NimBLE is a library the user
installs rather than part of a core — and there the sketch has to break the
cycle itself:

```cpp
#include <NimBLEDevice.h>   // before InstantIoT.h — this triggers discovery
#include <InstantIoT.h>
```

### Why plaintext is a type and not a flag

`Cloud(TOKEN)` returns a `SecureDestination`; `Cloud(TOKEN).plaintext()`
returns a `PlainDestination`. A different type means a different transport,
which means a different thing linked into the binary: 998 335 bytes with TLS
against 905 455 without, on ESP32. A board with no TLS stack at all can
therefore reach the cloud for real, instead of failing to compile over a path
it never takes.

Plaintext means plaintext: the token and the values travel readable. That is
a decision taken knowingly, never a default.

Two names were imposed by the compiler rather than chosen: `WiFiLink` because
the Arduino core already has a `WiFi` object, and `MyServer` because it also
has a `class Server`.

---

## 6. A signal leaving the board

```
InstantIoT.write(I0, 23.4f)
  │
  ▼
Facade::write — refuses (and logs once) if begin() has not run
  │
  ▼
InstantIoTCoreBase::write(SignalRef, float)
   • packs the value little-endian into 4 bytes
  │
  ▼
InstantIoTCoreBase::sendSignal(address, tag, payload, len)
   • THE CEILING — one global counter, not a table per address: the
     constraint comes from the platform, and a small board should not
     pay a table for it. Returns false, silently, when a call arrives
     too soon. That is not an error.
  │
  ▼
BinaryCodec::encodeSignal(...)  →  _transport.write(...)
```

### Why a second encoder

`BinaryCodec::encode` writes the widget id with `writeString`, which is
length-prefixed and NUL-terminated. An address of value 0 would be an empty
string. `encodeSignal` exists for that one reason: it lays the address down as
a raw byte.

### The frame

```
AA | VER | LEN(2) | DEV_COUNT=0 | WID_LEN=1 | addr | TYPE=0x20 | TAG | value | CRC
```

Nothing forked. `TYPE_SIGNAL` (0x20) is a type code like `TYPE_HEARTBEAT`
(0xFE) has always been, so the server's parser needs no branch of its own.

Two slots are reused rather than added: the address takes `WID` on one byte,
the value's type takes `EVENT`. Zero extra byte — 14 bytes for a float,
against 19 for the same measure named `"gauge1"`.

`DEV_COUNT` is 0: the board never repeats its own identity, since the
connection is already authenticated by its token.

Tags: `0x01` bool, `0x02` int32, `0x03` float, `0x04` text (48 bytes max).

### The contract is pinned by a test on the other side

The server repository holds a golden test asserting the exact 14 bytes of
`write(I5, 23.4f)`. The two repositories are compiled by different
toolchains; nothing else would catch a reordered field or a flipped
endianness — both would simply produce wrong values in somebody's history,
in silence.

If you change `encodeSignal`, that test must change with it, deliberately.

---

## 7. A signal arriving

```
_transport.read(...) → extractFrames() → processFrame(data, len)
  │
  ▼
BinaryCodec::decodeSignal(...)
   • DEV_COUNT must be 0, WID_LEN must be 1, TYPE must be 0x20
   • CRC checked only once those three hold
   • reads the address as the BYTE it is
   • the TAG's high bit (0x80) says "this is a restore", and is
     stripped from the tag before it is returned
  │
  ▼
decodeSignalValue(tag, payload, …) → SignalValue
   • int32 stays an integer, never round-tripped through a float
   • text is copied into the core's 49-byte buffer and terminated
  │
  ▼
onSignalWritten(e)      the catch-all, weak, overridable by the sketch
  │
  ▼
dispatchSignal(e, restore)
   • one uint8 compare per registered block — no strcmp anywhere
```

### Why the discriminator has to come first

`decode()` reads the `WID` slot with `readString`: a length byte, then that
many bytes. A signal puts a raw address there — so `I0` would read as an empty
string and `I4` would swallow the four bytes behind it, TYPE and TAG
included. `decodeSignal` therefore runs first and claims the frame or
declines it.

Declining is the delicate part, and it rests entirely on the TYPE byte: a
widget frame with no device list and a one-character id has *exactly* the same
shape as a signal. That case is in the host tests.

### A restore does not wake a gesture

On reconnect the server replays the last value of every signal that asks for
it. That is what a *state* needs — a setpoint, a threshold: `ISignal(I5,
float t)` must find it again.

But `ISimpleButton(I5)` declares something else: "I want to know that
somebody pressed". Nobody pressed. Delivering it would invent a gesture, and
the sketch would light a lamp nobody asked for.

The rule therefore belongs to the **block**, not to the signal's settings.
Whether replay is ticked or not, a gesture is not replayed. `SignalHandler`
carries a `gesture` flag for exactly this, set by the `I<Widget>` macros and
left false by `ISignal`.

---

## 8. The DSL — one, not two

There is one style. The callback form (`void onSimpleButtonEvent(…)` with
`ON_PRESS("btn1")` guards) is gone, along with the widget ids it addressed.

**The head carries the data, `WHEN_` carries the kind.**

One kind of event, and the parameters are enough:

```cpp
IJoystick(I2, float x, float y)  { driveMotors(x, y); };
IHorizontalSlider(I1, float v)   { analogWrite(PWM, v); };
IVerticalSlider(I4, float v)     { … };
ISwitch(I3, bool on)             { digitalWrite(RELAY, on); };
ISegmentedSwitch(I5, int index)  { mode = index; };
ISignal(I6, const char* text)    { … };
```

Several kinds, and `WHEN_` sorts them — which is what stops the sketch from
growing a staircase of `if`:

```cpp
ISimpleButton(I0) {
    WHEN_PRESSED       { … }
    WHEN_RELEASED      { … }
    WHEN_LONG_PRESSED  { … }
};

IDirectionPad(I7) {
    WHEN_UP           { forward(); }        // one key, one gesture
    WHEN_UP_LONG      { faster(); }
    WHEN_RELEASED_ANY { stop(); }
    WHEN_PAD_PRESSED(key) { … }             // every key, same treatment
};

IEmergencyButton(I8) {
    WHEN_TRIGGERED { … }
    WHEN_RESET     { … }
};
```

Blocks live at **file scope** and close with `};`. Each expands to a static
function plus a `SignalRegistrar` whose constructor links a ~12-byte node
into a global intrusive list, before `setup()` runs. Nothing on the heap.

Several blocks may watch one address — an `ISignal(I0, float v)` next to an
`ISimpleButton(I0)`. Both run, except on a restore (see §7).

### What the board reads

A gesture travels as a **value**, by convention: **1 pressed, 0 released, 2
long-pressed**. A joystick writes `"0.42,-0.15"`, a direction pad writes
`"UP"`, `"UP_LONG"`, `"UP_RELEASE"`. The decoding lives in
`SignalToWidget.hpp` — written once instead of in every sketch.

### The handler is not typed, the capture is

The board cannot know what the server declared for an address, so
`SignalValue` carries every reading of itself and the sketch picks one. The
conversion operator is a template, so any target type — `uint8_t` included —
is an exact match rather than an ambiguity between a handful of fixed
operators. `bool` and text have their own conversions: 0.5 must read as
*true* and not truncate to 0 first, and a non-empty text is true.

---

## 9. Transports — the `ITransport` contract

```cpp
struct ITransport {
    virtual bool   begin() = 0;                            // open
    virtual void   poll() = 0;                             // service the stack
    virtual bool   connected() = 0;
    virtual int    available() = 0;
    virtual int    read(uint8_t* buf, size_t len) = 0;     // -1 on error
    virtual size_t write(const uint8_t* buf, size_t len) = 0;
};
```

The core never sees WiFi or BLE — only this interface. Adding a physical
medium is: implement `ITransport`, then add the link that builds it in
`Links.hpp`.

The TCP and TLS clients add three setters the links call before `begin()`:
`setCredentials(ssid, pass)`, `setHeartbeat(ms)`, and — on the TLS ones only
— `setCACert(pem)` / `setInsecure()`.

### Two rules a WiFi transport must keep

**Never restart an association that is in flight.** `WiFi.begin()` on a
connecting station does not restart it, it kills it — the ESP32 says so
(`sta is connecting, cannot set config`), and a board on a slow network then
never arrives, because every retry interrupts the attempt just before it
completes. `poll()` records when the last `WiFi.begin` happened and watches
without touching until the window has passed. `WiFi.begin` is called from one
place only, `lanceLaTentativeWiFi()`, so the rule is enforceable.

**Say why it failed, out loud.** The ESP32 emits a disconnect reason within
about two seconds; `RaisonWiFi_ESP32.hpp` catches it and prints it in plain
words — once per distinct reason, and *not* behind `INSTANTIOT_DEBUG`. It is
the one moment where the board can do nothing else and the person watching
has no other source of truth. A password that lost two characters in a
copy-paste cost an hour of blind debugging before this existed.
`INSTANTIOT_QUIET` silences it.

---

## 10. Memory model

Statically allocated end to end.

| Buffer | ESP32 | R4 / ESP8266 | other |
|---|---|---|---|
| `_rxBuffer` | 2 KB | 1 KB | 512 B |
| `_txBuffer` | 1 KB | 512 B | 256 B |

Plus a `char[49]` in the core for an incoming text signal, and one ~12-byte
static node per `I<Widget>` / `ISignal` block. No `String`, no
`std::string`, no heap.

---

## 11. Compile-time configuration — `InstantIoTConfig.h`

```cpp
#define INSTANTIOT_DEBUG               0   // 1 → IIOT_LOG to Serial
#define INSTANTIOT_DEFAULT_SIGNAL_RATE 50  // frames/s ceiling until the
                                           // server pushes the real one
#define INSTANT_RX_BUFFER_SIZE         …   // see the table above
#define INSTANT_TX_BUFFER_SIZE         …
#define INSTANT_AP_PORT                8080
```

Destination defaults live in `Destinations.hpp` and are overridable the same
way: `INSTANTIOT_CLOUD_HOST`, `INSTANTIOT_CLOUD_TLS_PORT` (9443),
`INSTANTIOT_CLOUD_PLAIN_PORT` (9001), `INSTANTIOT_DEFAULT_HEARTBEAT_MS`
(5000).

The rate ceiling is **cooperative**: the server never trusts it, its own fuse
stays. It exists so a beginner writing in the main loop is not disconnected
for flooding.

---

## 12. Heartbeat

Server modes only — it has no meaning in access-point or BLE mode, where
there is no server to reassure.

```
TYPE = 0xFE (HEARTBEAT)   WID_LEN = 0   EVENT = 0   PAYLOAD = {}
```

The interval is plumbed on two layers, both from the destination: the
transport announces it at the handshake, the core emits it in `loop()`. The
server sets its socket read timeout to about 2.5× the announced interval
(clamped to 2 s … 120 s) and flags the device offline after that silence.
Heartbeats are never relayed to apps.

Change it with `Cloud(TOKEN).heartbeatEvery(20000)`, before `begin()`.

---

## 13. The host bench

```
./test/host/run.sh
```

Compiles five test files with a plain `g++` against a 66-line `Arduino.h`
shim and runs them in about a second: no board, no IDE. It
does not replace a run on real hardware; it catches what does not need
hardware to be wrong.

| File | What it pins |
|---|---|
| `test_signals.cpp` | the golden frames, byte for byte, through the real `processFrame` |
| `test_singleton.cpp` | calling before `begin()`, writing from inside a block, a second `begin()` |
| `test_destinations.cpp` | the defaults a sketch gets without asking |
| `decodeurs.cpp` | the gesture convention, positions, pad names |
| `dsl_sur_signaux.cpp` | that the macros expand and register |

The frames are routed through the real `processFrame`, not a copy of it —
a copy keeps passing on the day the original changes.

What it has caught, in being written: `e.text()` where `text()` belongs to
`SignalValue`; `ISignal` registered as a gesture block; a `write` before
`begin()` segfaulting; and `dispatchSignalFrame` delivering *only* restores,
which had left every `ISignal` and every `ISimpleButton` on the board silent.
All of them compiled. No review would have seen them.

Compiling for real needs `arduino-cli`:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/commandes/Croix
```

The three supported targets are `esp32:esp32:esp32`,
`arduino:renesas_uno:unor4wifi` and `esp8266:esp8266:nodemcuv2`. A Bluetooth
sketch needs the `huge_app` partition scheme — Bluetooth Classic plus WiFi
does not fit in the default 1.3 MB, and never did.

PlatformIO works with no configuration of its own: drop the library in
`lib/`, and its dependency finder resolves the rest. Verified on `esp32dev`
(the DSL example and the TLS one) and on `uno` (`SerialLink`, same 6 554
bytes as arduino-cli).

---

## 14. Onboarding — where to start

1. `src/InstantIoT.h` — the singleton and the three `begin` forms
2. `src/Links.hpp` + `src/Destinations.hpp` — what a sketch chooses
3. `src/core/InstantIoTCore.hpp` — the main loop and RX assembly
4. `src/core/BinaryCodec.hpp` — the wire protocol, byte by byte
5. `src/core/SignalEvents.hpp` — the handler list and dispatch
6. `src/utils/InstantIoTWhen.hpp` — the macros a sketch writes
7. `src/transport/wifi/SoftAP_ESP32.hpp` — a reference `ITransport`

### Adding a transport

1. Implement `ITransport` in `src/transport/<medium>/<Name>.hpp`.
2. Add the platform branch and the feature macro in `Links.hpp`.
3. Give it a link struct with `transport()` (direct) or `transportVers(dest)`
   (reaching a destination), and a `_IIO_LIAISON_ABSENTE` shell for the
   platforms that lack it.
4. If it needs a library outside the core, read §5's `__has_include` trap
   before guarding it.
5. Add the name to the `using` block at the bottom of `InstantIoT.h`, so the
   sketch does not need `iiot::`.
6. Compile one example per supported board before claiming it works.

---

## 15. Known debt

Written down rather than left to be rediscovered.

- **`test/host/test_events.cpp` is out of the bench.** It asserts
  `typeAtAddress(5) == TYPE_SIMPLEBUTTON` and `ISimpleButton("btn1")`, both
  removed on purpose. Making it green again means deciding what an EVENT
  becomes on the board — work, not a touch-up.
- **Five orphaned event structs.** Removing the dead `WHEN_` macros left
  `SwitchEvent`, `JoystickEvent`, `HorizontalSliderEvent`,
  `VerticalSliderEvent` and `SegmentedSwitchEvent` in
  `InstantIoTMessage.hpp` with nothing building them — their blocks carry
  the value in the head now. Their `*EventKind` enums go with them.
  `ButtonEventKind`, `DPadButton` and `DPadEventKind` stay: `SignalToWidget`
  decodes into them.
- **Dead `INSTANTIOT_WIDGETS_*` flags** in `InstantIoTConfig.h`. They gated
  a `src/widgets/` directory that no longer exists.
- **No Ethernet transport.** The model expects it — `begin(EthernetLink(),
  Cloud(TOKEN))` would need no change anywhere else — but nothing is written
  until it has run on a board.
- **`README.md` still describes the erased model.**
- **No disconnect reason on Uno R4.** WiFiS3 has no event API, so the
  `RaisonWiFi_ESP32` treatment stops at the ESP32. An R4 that cannot join a
  network still says only "WiFi timeout".

---

**In one sentence:** the board writes values at addresses and reacts to the
values written to it, over any `ITransport`, with zero dynamic memory — and
what those values look like is not its business.
