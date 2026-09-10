# InstantIoT Arduino Library — Architecture

> A header-only C++ library that connects an Arduino board (ESP32 / ESP8266 /
> Uno R4 WiFi) to the InstantIoT mobile app over a compact binary protocol.
>
> No dynamic memory, no JSON, fully static allocation.
>
> Read this end to end (~10 min) before contributing.

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
│   ├─ BinaryCodec.hpp          encode/decode — frames and signals
│   ├─ InstantIoTCore.hpp       main loop: RX assembly, TX, heartbeat
│   ├─ InstantIoTSignals.hpp    SignalRef, the I0..I255 constants
│   ├─ SignalEvents.hpp         SignalValue, the handler list, dispatch
│   ├─ SignalToWidget.hpp       gesture/position/pad decoding
│   ├─ InstantIoTMessage.hpp    typed event structs (SimpleButtonEvent…)
│   └─ InstantIoTDeviceConfig.hpp
│
├─ transport/
│   ├─ TcpSession.hpp                    ★ everything a TCP link is NOT
│   ├─ wifi/SoftAP_{ESP32,ESP8266,NINA,R4}.hpp    board hosts its own WiFi
│   ├─ wifi/TcpClient_{ESP32,ESP8266,NINA,R4}.hpp plain TCP to a server
│   ├─ wifi/TlsClient_{ESP32,ESP8266,NINA,R4}.hpp TLS to a server
│   ├─ wifi/WiFiReason_ESP32.hpp              why the association failed
│   ├─ ethernet/EthClient_W5x00.hpp       the W5500's own TCP stack, plain
│   ├─ ethernet/EthLink_ESP32.hpp         the same chip behind lwIP, TLS too
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
separates the two is the encryption, not the wire. That is exactly where the
Ethernet implementations slotted in, later, without the naming having to move.

**`TcpSession.hpp` is the one file to read first in this directory.** Every
TCP transport above is a subclass of it that answers two questions — *is the
link up?* and *bring it up* — and inherits the rest: session, handshake,
retry, backoff with jitter, the in-flight rule, the four `ITransport`
methods. `TcpClient_ESP32` went from 363 lines to 132 the day it appeared.

The two Ethernet files are not a duplicate. They drive the same W5500 two
different ways, and only one of them can carry TLS — see §5, note 4.

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
about each other, and that is what let `EthernetLink()` arrive later without
one line changing on the destination side. It did.

```cpp
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
InstantIoT.begin(AccessPoint("MyBoard", "12345678"));   // no far end to name
                                                         // 8 characters or more, or nothing starts
```

Both are **descriptions**, not live objects: they do not survive the
statement. The facade builds the transport from them and keeps it in a
function-local `static`.

| | ESP32 | Uno R4 WiFi | NINA¹ | ESP8266 | AVR (Mega) |
|---|---|---|---|---|---|
| `AccessPoint` | ✓ | ✓ | ✓ | ✓ | — |
| `WiFiLink` | ✓ | ✓ | ✓ | ✓ | — |
| `EthernetLink` | ✓ | ✓ | ✓ | ✓ | ✓ |
| `Cloud` / `MyServer` plain | ✓ | ✓ | ✓ | ✓ | via Ethernet |
| `Cloud` / `MyServer` TLS, WiFi | ✓ | ✓ | ✓, fixed roots² | ✓, ~20 KB heap³ | no radio |
| `Cloud` / `MyServer` TLS, Ethernet | ✓, via lwIP⁴ | — | not done⁵ | — | **never** |
| `BluetoothLink` (Classic) | ✓ ⁶ | — | — | — | — |
| `BLELink` (NimBLE) | ✓ ⁷ | — | — | — | — |
| `SerialLink` | — | — | — | ✓ | ✓ |

¹ MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2 — one u-blox NINA-W10 module,
one branch in `Links.hpp`.
² The trust store is in the module firmware and a sketch cannot add to it.
See the header of `TlsClient_NINA.hpp`.
³ BearSSL in software: 104 KB of flash, ~20 KB of heap per session, and an
NTP round trip before the first handshake. Measured, not estimated — the
numbers and the probe that produced them are in the header of
`TlsClient_ESP8266.hpp`.
⁴ Not through Arduino's `Ethernet` library, which uses the TCP stack wired
inside the W5500 — a socket a TLS client cannot wrap. The ESP32 core drives
the same chip behind lwIP instead. See `EthLink_ESP32.hpp`.
⁵ Possible with `ArduinoBearSSL` over an `EthernetClient` (Blynk's recipe),
measured and not written: it fits on the two SAMD boards at 74–76 % of SRAM
and overflows the flash of the Uno WiFi Rev.2. It would also mean shipping
the roots a second time as `br_x509_trust_anchor` structs. See README note 7.
⁶ The original ESP32 only. The S3, C3 and C6 have BLE and no Bluetooth
Classic radio; the S2 has neither.
⁷ The whole family except the S2. The sketch must `#include <NimBLEDevice.h>`
BEFORE `<InstantIoT.h>`, and both need the `huge_app` partition scheme.

**The NINA three** were exactly that prediction, and it held: one `ITransport`
over `WiFiNINA` plus one branch, and nothing in the protocol or the DSL stood
in the way. (Something used to: a single `dtostrf` call in `BinaryCodec` made
the library uncompilable on SAMD, because that function comes from avr-libc.)

They are grouped by their **radio** here and by their **SRAM** in
`InstantIoTConfig.h` — the Uno WiFi Rev.2 shares a module with a MKR 1010 and
has five times less memory. Two questions, two groupings; sorting by the radio
in both places would have handed an ATmega4809 the ration of a chip its
senior.

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

### A frame the board cannot read

`processFrame` counts it and says so once. It used to drop it in silence,
which is how the removal of the EVENT path went unnoticed: the app kept
sending `TYPE_EVENT` frames, the relay kept forwarding them, and the board
kept swallowing them.

```cpp
InstantIoT.write(I9, InstantIoT.ignoredFrames());   // readable from the app
```

`ignoredFrames()` is readable from a sketch, so the symptom can be published
on a signal and seen from the app rather than guessed at over a serial cable.
Zero is the normal answer; a counter that climbs means the other end is
sending something this version does not understand.

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

### `TcpSession` — what a TCP transport does NOT have to write

Ten of the transports above are TCP. Written one by one they were the same
file ten times: session, handshake, retry, backoff with jitter, the in-flight
rule, the four `ITransport` methods. `TcpSession` is all of that, once, and a
subclass answers only what genuinely differs:

| seam | default | who overrides it, and why |
|---|---|---|
| `linkUp()` | — | required: `WiFi.status()`, `ETH.linkUp()`, an IP lease |
| `beginLink()` | — | required. Returns **true if an attempt is now in flight**, false if it is already over. A WiFi association returns true; a blocking DHCP returns false |
| `prepareClient()` | nothing | the ESP32 wants a timeout and `setNoDelay`; the Uno R4 must NOT get a timeout at all |
| `linkCredentialsReady()` | yes | Ethernet needs none; WiFi refuses to start without an SSID |
| `readyToConnect()` | yes | the ESP8266's TLS is the first link with a precondition beyond an address: BearSSL refuses a certificate it cannot date |
| `onLinkDown/Up()`, `endLinkAttempt()` | nothing | only the ESP32 can say *why*; only WiFi needs an explicit `disconnect()` |

The return value of `beginLink()` is the one that costs when it is wrong:
backwards, it buys either a busy loop or a fifteen-second stall.

### Two rules the trunk keeps for everyone

**Never restart an attempt that is in flight.** `WiFi.begin()` on a
connecting station does not restart it, it kills it — the ESP32 says so
(`sta is connecting, cannot set config`), and a board on a slow network then
never arrives, because every retry interrupts the attempt just before it
completes. It looks WiFi-specific and is not: `ETH.begin` re-initialises the
driver, so calling it again cuts the very cable it is trying to restore.
`TcpSession::startLinkAttempt()` is the only place an attempt starts, which
is what makes the rule enforceable at all.

**Say why it failed, out loud** — where the chip can say it. The ESP32 emits a disconnect reason within
about two seconds; `WiFiReason_ESP32.hpp` catches it and prints it in plain
words — once per distinct reason, and *not* behind `INSTANTIOT_DEBUG`. It is
the one moment where the board can do nothing else and the person watching
has no other source of truth. A password that lost two characters in a
copy-paste cost an hour of blind debugging before this existed.
`INSTANTIOT_QUIET` silences it.

Only the ESP32 core reports one. WiFiS3 (Uno R4), WiFiNINA and the ESP8266
core have no equivalent event API, so on those four families a board that
cannot join says only that it timed out. That is a gap in the hardware's
vocabulary, not in this file.

---

## 10. Memory model

Statically allocated end to end.

| Buffer | ESP32 | R4 / ESP8266 / SAMD | AVR, megaAVR |
|---|---|---|---|
| `_rxBuffer` | 2 KB | 1 KB | 256 B — four whole frames |
| `_txBuffer` | 1 KB | 512 B | 128 B — two |

The small column is sized by the **protocol**, not by a guess: the largest
frame the 2.0 model can produce is a 48-character text signal, fifty-eight
bytes with its header and CRC. The old defaults were cut for widgets with
long strings and multi-series charts, a class of message that no longer
travels — on an Uno that cost 624 bytes of SRAM, 80 % → 61 % measured, for a
capacity nothing could ever use.

The dividing line is SRAM, not the radio: the Uno WiFi Rev.2 shares a WiFi
module with a MKR 1010 and has 6 KB against its 32, so it sits in the right
column here and with the NINA boards in `Links.hpp`.

Plus a `char[49]` in the core for an incoming text signal, and one ~12-byte
static node per `I<Widget>` / `ISignal` block. No `String`, no
`std::string`, no heap.

---

## 11. Compile-time configuration — `InstantIoTConfig.h`

```cpp
#define INSTANTIOT_DEBUG               0   // 1 → IIOT_LOG to Serial
#define INSTANTIOT_DEFAULT_SIGNAL_RATE 10  // frames/s ceiling, burst of 2x;
                                           // mirrors the relay fuse (10/s, 20)
#define INSTANT_MAX_FRAME_SIZE         64  // the protocol's largest frame
#define INSTANT_RX_BUFFER_SIZE         …   // see the table above
#define INSTANT_TX_BUFFER_SIZE         …
#define INSTANT_AP_PORT                8080
```

Four more are opt-in rather than tuning, and each belongs to one platform:

```cpp
#define INSTANTIOT_ETHERNET        1   // BEFORE the include — see §5
#define INSTANTIOT_TLS_RX_BUFFER   6144  // ESP8266: sized against a 3411-byte
                                         // Certificate record. Read the header
                                         // of TlsClient_ESP8266.hpp first
#define INSTANTIOT_SNTP_SERVER  "pool.ntp.org"   // ESP8266: BearSSL refuses a
                                                 // certificate it cannot date
#define INSTANTIOT_ETH_PHY_ADDR    1   // ESP32 + W5500 behind lwIP
```

`INSTANTIOT_ETHERNET` is opt-in and cannot be detected: Arduino's build finds
libraries by READING `#include` directives, so a conditional include is never
seen and the library never reaches the path. Asking the sketch to say so is
the honest form.

The platform is detected, not configured — `INSTANTIOT_PLATFORM_{ESP32,
ESP8266, R4, SAMD, MEGAAVR, AVR}`. A board that matches none of them still
compiles and gets a `#warning` naming the ones the bench actually covers.

Destination defaults live in `Destinations.hpp` and are overridable the same
way: `INSTANTIOT_CLOUD_HOST`, `INSTANTIOT_CLOUD_TLS_PORT` (9443),
`INSTANTIOT_CLOUD_PLAIN_PORT` (9001), `INSTANTIOT_DEFAULT_HEARTBEAT_MS`
(5000).

The rate ceiling is **cooperative**: the server never trusts it and never
pushes it, its own fuse (10 frames/s, burst 20) stays. The board applies the
same figures as a token bucket: a burst of a few writes in a row passes, a
`loop()` writing without a delay is slowed to the rate. It exists so a
beginner writing in the main loop is not disconnected for flooding. The
transports back it up on reconnection: after a drop the first retry waits
its jittered backoff, and the backoff only resets once a session has held
`INSTANTIOT_SESSION_HELD_MS` (10 s), so a board the relay keeps rejecting
slows down to one attempt every 30 s instead of one a second.

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

Change it with `Cloud(TOKEN).heartbeatEvery(20000)`, before `begin()`. The
value is held to 1 s … 48 s: below, the board spends its budget on
presence; above, the server's window stops following and the board is cut
at 120 s while believing itself on time. Zero is not a choice — on a relay
it was a session cut every 90 s.

---

## 13. The host bench

```
./test/host/run.sh
```

Compiles six test files with a plain `g++` against a small `Arduino.h`
shim and runs them in about a second: no board, no IDE. It
does not replace a run on real hardware; it catches what does not need
hardware to be wrong.

| File | What it pins |
|---|---|
| `test_signals.cpp` | the golden frames, byte for byte, through the real `processFrame` |
| `test_singleton.cpp` | calling before `begin()`, writing from inside a block, a second `begin()`, a link that opens badly, an unreadable frame |
| `test_destinations.cpp` | the defaults a sketch gets without asking |
| `decoders.cpp` | the gesture convention, positions, pad names |
| `dsl_on_signals.cpp` | that the macros expand and register |
| `heartbeat_frame.cpp` | that the beat carries no invented board name, and the one a sketch chooses |

The frames are routed through the real `processFrame`, not a copy of it —
a copy keeps passing on the day the original changes.

What it has caught, in being written:

- `e.text()` where `text()` belongs to `SignalValue`, not to `SignalEvent`
- `ISignal` registered as a GESTURE block, so a restore no longer handed it
  back its setpoint
- a `write` before `begin()` segfaulting — on a board, a reboot
- `dispatchSignalFrame` delivering *only* restores, which had left every
  `ISignal` and every `ISimpleButton` on the board silent
- a board that gave up for good after one failed `begin()`
- a frame the board cannot read vanishing without a word
- sixteen configuration flags removed as dead when one of them still gated a
  `case` — the build broke on `unused variable 'p'` in the same minute

All of them compiled. No review would have seen them.

### The board bench — `test/boards.sh`

The other half, and the slow one. It compiles **every example on every
board**: 22 sketches × 11 boards = 242 pairs, about an hour cold.

```
bash test/boards.sh     →  193 green · 49 expected failures · 0 surprises
```

The expected failures are a list with a reason on each line, and the rule
that gives the bench its value is symmetrical: **an unexpected GREEN is a
surprise too**. It has already caught one — a sketch listed as failing on
seven boards that compiled on all of them, and the mistake was in the list,
not in the code.

A line leaves that list when the code earns it. `esp8266 / TheCloud` sat
there through two stages waiting on a TLS stack, and left when BearSSL was
measured rather than guessed at.

Two sketches need a board option, which the bench passes as an FQBN suffix
and their headers repeat: `BluetoothClassic` and `BluetoothLE` want
`PartitionScheme=huge_app` — the Bluetooth stack needs 1.60 MB and the
default application partition is 1.31.

The eleven targets are the five ESP32 variants (`esp32`, `esp32s2`,
`esp32s3`, `esp32c3`, `esp32c6`), `esp8266:esp8266:nodemcuv2`,
`arduino:renesas_uno:unor4wifi`, `arduino:avr:mega`,
`arduino:samd:mkrwifi1010`, `arduino:samd:nano_33_iot` and
`arduino:megaavr:uno2018`.

PlatformIO works with no configuration of its own: drop the library in
`lib/`, and its dependency finder resolves the rest. Verified on `esp32dev`
(the DSL example and the TLS one) and on `uno` (`SerialLink`), where it
produces the same binary size as arduino-cli.

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

- **Almost nothing here has run on a board.** One thing has: an ESP32 in
  access-point mode, receiving signal frames from the app, dispatching them to
  a `IJoystick` block. That validated the frame, the DSL and the transport —
  and it found a bug on the app side, not here.
  Everything else is compiled and measured, never plugged in: both Ethernet
  transports — including the encrypted one, which exists on ESP32 only — the
  ESP8266's BearSSL and its NTP round trip, the three NINA boards, Bluetooth,
  BLE, the serial link. `test/boards.sh` says the code
  builds and the footprints fit; it says nothing about a handshake. This stays
  at the top of the list until the hardware is on the desk.
- **The ESP32 does not check a certificate's dates.** Its mbedTLS is built
  with `CONFIG_MBEDTLS_HAVE_TIME=y` but `CONFIG_MBEDTLS_HAVE_TIME_DATE`
  *not set* — read in the `sdkconfig` of the installed cores (esp32 and
  esp32-s3 libs 3.3.7, and the Arduino ESP32 2.0.18 core alike), not
  assumed. The chain is still verified against the embedded Let's Encrypt
  roots, so a stranger's certificate is refused; but an **expired** one
  that was once legitimate is accepted for as long as its key exists.
  With Let's Encrypt's 90-day lifetime the window is short, and the
  library cannot re-enable the check — it is compiled out of the core. The
  ESP8266 does date its certificates (BearSSL insists, hence its NTP round
  trip). What would close it: a core built with the flag, or a check of
  `notAfter` in the sketch once the clock is set — neither is this
  library's to do today.
- **Connecting blocks `loop()`, for up to fifteen seconds.** `connect()`
  is synchronous on every Arduino stack: the TLS handshake is bounded to
  10 s on ESP32 (`INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S`), and on the Uno R4
  it cannot be bounded at all — a non-zero timeout switches the modem to a
  command that misbehaves (see `TcpClient_R4.hpp`). A relay that is down
  therefore freezes the sketch on each attempt, and the backoff is what
  keeps those attempts rare (one every 30 s once the ceiling is reached).
  A sketch that cannot afford the pause should not drive a motor from the
  same `loop()` as its link.
- **`SignalToWidget.hpp` is misnamed.** There are no widgets on the board any
  more — that was the point of 2.0. What it does is turn a raw value into
  what a *block* expects. The name will send someone looking in the wrong
  place; it was left alone deliberately, since no sketch names it.
- **`decodeEvent` is gone, and so is the app's `buildEvent`.** A gesture now
  travels as a value, by the 1 / 0 / 2 convention, and there is only one way
  to send a press. The relay no longer knows about EVENT either.
- **TLS over a cable exists on one family only, and not for want of trying.**
  On the NINA boards it is possible with `ArduinoBearSSL` over an
  `EthernetClient` — Blynk's recipe, read in their source rather than assumed.
  It was measured and deliberately not written: a realistic sketch takes
  74–76 % of the SRAM of the two SAMD boards, leaving about 8 KB for a stack
  that a BearSSL handshake eats into, and it overflows the flash of the Uno
  WiFi Rev.2 outright. It would also mean shipping the Let's Encrypt roots a
  **second time**, as `br_x509_trust_anchor` structs rather than PEM, and
  keeping both in step — that chain changed once already this month. What
  would settle it is a MKR on the desk, not more reading.
- **`INSTANTIOT_MAX_WIDGET_ID_LENGTH` carries a name from before 2.0.** It
  sizes the device name, the device id, the dashboard id and the legacy
  codec's WID slot — none of which is a widget. Renaming it would break a
  sketch that redefined it, so the name stayed and the header says why.
- **No disconnect reason outside the ESP32.** Only its core reports WHY an
  association failed. WiFiS3 (Uno R4), WiFiNINA and the ESP8266 core have no
  equivalent event API, so `WiFiReason_ESP32` stops where it is named. On
  those four families a board that cannot join says only "link timeout".
- ~~**The legacy codec is half-unreachable.**~~ **Done.** `BinaryCodec::decode`
  and `decodePayload` are gone, and the sixteen `INSTANTIOT_WIDGETS_*` flags
  with them: each one gated a `case` of that decoder and nothing else, so an
  unreachable road made unreachable switches. `Codec.h` and `DecodedMessage`
  went too, having no other reader.

  `encode` stays — the heartbeat still uses it — and so do `encodeSignal` and
  `decodeSignal`, which are the 2.0 path. A sketch that redefined one of the
  flags still compiles: a `#define` nobody reads does nothing.

  It was a decision about the public surface of a header, not a cleanup, and
  the decision was taken: **2.0 carries no backward compatibility.** Whoever
  needs the old protocol keeps the old library and a matching APK.

---

**In one sentence:** the board writes values at addresses and reacts to the
values written to it, over any `ITransport`, with zero dynamic memory — and
what those values look like is not its business.
