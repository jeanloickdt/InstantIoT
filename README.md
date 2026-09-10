# ⚡ InstantIoT Arduino Library

**Your board writes a value. The app decides what it looks like.**

Connect an ESP32, ESP8266 or Arduino Uno R4 WiFi to the
[InstantIoT mobile app](https://instantiot.io) — directly over the board's own
Wi-Fi, through a server you host, or through the InstantIoT Cloud over TLS.

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

One header. One object. One `begin`.

---

## The idea in one sentence

A sketch writes a value at an **address** — `I0`, `I5`, like `A0`. Whether that
value is drawn as a gauge, a chart or a plain number is decided **in the app**,
and changed without reflashing. The board carries no widgets, no widget ids,
and no drawing code.

The same addresses work in the other direction: the app writes, the board
reacts.

---

## Install

### Arduino Library Manager
**Sketch → Include Library → Manage Libraries**, search **InstantIoT**, install.

### Manual
Download a release and extract it into your Arduino `libraries/` folder.

---

## Connecting

`begin()` reads in the order things happen: the board joins a network, then
reaches a destination.

```cpp
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));                 // TLS
InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
InstantIoT.begin(AccessPoint("MyBoard", "12345678"));   // the board IS the network — pick your own
                                                         // password, 8 characters or more, or nothing starts
InstantIoT.begin(BluetoothLink("MyBoard"));
InstantIoT.begin(SerialLink(10, 11));
```

Three of them have no destination to name — the phone is at the other end of
the wire.

| | ESP32 | Uno R4 WiFi | NINA¹ | ESP8266 | AVR (Mega) |
|---|:---:|:---:|:---:|:---:|:---:|
| `AccessPoint` | ✅ | ✅ | ✅ | ✅ | — |
| `WiFiLink` + `Cloud` / `MyServer` | ✅ | ✅ | ✅ | ✅ (clair) | — |
| `EthernetLink` + `Cloud` / `MyServer` | ✅ | ✅ | ✅ | ✅ | ✅ |
| TLS sur WiFi | ✅ | ✅ | ✅ ² | ✅ ³ | — pas de radio |
| **TLS sur Ethernet** | ✅ ⁶ | — | — ⁷ | — | **jamais** |
| `BluetoothLink` (Classic) | ✅ ⁴ | — | — | — | — |
| `BLELink` (NimBLE) | ✅ ⁵ | — | — | — | — |
| `SerialLink` (SoftwareSerial) | — | — | — | ✅ | ✅ |

¹ **NINA** = MKR WiFi 1010, Nano 33 IoT, Uno WiFi Rev.2 — trois cartes dont
le WiFi est le même co-processeur u-blox NINA-W10.

² **Le TLS des NINA valide, mais avec des racines qu'on ne choisit pas.**
Elles vivent dans le firmware du module et un croquis ne peut pas en
ajouter : `withCertificate()` y est ignoré, en le disant. Soit la racine de
votre serveur y est — celle de Let's Encrypt l'est, sur un firmware
récent — soit il faut passer par le *WiFiNINA Firmware Updater* de l'IDE,
soit `.plaintext()`.

⁴ **Bluetooth Classic : l'ESP32 d'origine, et lui seul.** Le S3, le C3 et
le C6 n'ont que le BLE ; le S2 n'a aucune radio Bluetooth. Sur ces
quatre-la, `BluetoothLink` ne compile pas, et le message le dit.

⁵ **BLE : toute la famille sauf le S2**, qui n'a pas la radio. Le croquis
doit ecrire `#include <NimBLEDevice.h>` AVANT `<InstantIoT.h>` — voir le
tableau ci-dessous.

⁶ **Le TLS sur cable n'existe que sur ESP32, et pas pour la raison qu'on
croit.** Ce n'est pas que le W5500 manque de crypto : le chiffrement
tournerait sur le processeur. C'est que la bibliotheque `Ethernet`
d'Arduino se sert de la pile TCP **cablee dans le composant**, et qu'un
client TLS des ESP n'enveloppe pas un `Client`, il en EST un
(`class NetworkClientSecure : public NetworkClient`). Il n'y a rien a
envelopper.

Le coeur ESP32 sait piloter le meme W5500 comme une carte reseau,
derriere lwIP — `ETH.begin(ETH_PHY_W5500, ...)` — et alors le TLS marche
sans savoir qu'il y a un cable. C'est ce que fait `EthLink_ESP32`, et
c'est pourquoi cette forme demande les trois broches : un module n'a
aucun brochage impose.

⁷ **Sur les NINA, c'est possible mais pas fait, et les chiffres sont
mesures.** Il faudrait `ArduinoBearSSL`, qui se compose sur un `Client`
quelconque — la recette de Blynk. Un croquis realiste tient sur les deux
SAMD (MKR 1010 : 48 % de flash, **74 % de SRAM**, 8,4 Ko restants ; Nano
33 IoT : 76 %, 7,8 Ko) et **ne tient pas** sur l'Uno WiFi Rev.2, qui
deborde la flash. Deux couts s'ajoutent : `BearSSLClient` prend des
`br_x509_trust_anchor`, pas du PEM — donc nos racines en DEUX
representations a garder d'accord — et il faut une horloge NTP comme sur
l'ESP8266. Ca se decidera avec une carte sur le bureau, pas avant.

³ **Le TLS de l'ESP8266 est logiciel, et il se paie.** BearSSL prend
104 Ko de flash et environ 20 Ko de tas pendant qu'une session est
ouverte, sur une carte qui en a a peu pres 40 Ko de libres. Ca tient — un
croquis qui garde lui-meme 10 Ko en memoire, non. Il lui faut aussi une
horloge : la bibliotheque interroge un serveur NTP toute seule au moment
ou le WiFi monte, parce qu'un certificat a des dates et qu'une carte qui
demarre croit etre en 1970. Les mesures sont en tete de
`src/transport/wifi/TlsClient_ESP8266.hpp`.

`EthernetLink` demande un shield W5100 / W5500 et **une ligne avant
l'include** : `#define INSTANTIOT_ETHERNET 1`. Elle n'est pas détectée
toute seule — le build Arduino trouve les bibliothèques en lisant les
`#include`, donc un include conditionnel n'est jamais vu.

**Sur AVR, la colonne TLS ne se remplira pas.** Le W5x00 ne porte pas de
crypto et un AVR n'a ni la RAM ni le flash pour une poignée de main. Un
Mega atteint le cloud en clair, et le compilateur le dit plutôt que de
laisser le croquis le découvrir sur l'établi. **Un Uno, lui, n'a pas
assez de RAM pour l'Ethernet du tout** — mesuré : la bibliothèque seule
occupe 61 % de ses 2 Ko.

A combination your board cannot do **fails to compile**, and the message says
what to write instead:

```
error: static assertion failed: AccessPoint is already the far end: the board
IS the network, and the app connects straight to it. To reach a server, the
link is WiFiLink(ssid, password).
```

### TLS, and how to step out of it

`Cloud(TOKEN)` encrypts and verifies the server's identity against the
embedded Let's Encrypt roots. Three ways out, and they do not mean the same
thing:

```cpp
Cloud(TOKEN).withCertificate(MY_ROOT)  // your own authority — still verified
Cloud(TOKEN).withoutCertCheck()        // encrypted, identity unchecked — bring-up only
Cloud(TOKEN).plaintext()               // no encryption at all
```

`plaintext()` returns a **different type**, so no TLS stack is linked at all:
898 975 bytes against 991 947 on an ESP32 — 93 KB the board never has to
carry. A board with no TLS can reach the
cloud for real, instead of failing to compile over a path it never takes. The
token then travels readable — that is the price, and it is a decision, never a
default.

Self-hosting with your own certificate: `MyServer(host, port, TOKEN).secure()`.

---

## Receiving — the DSL

Blocks live at **file scope** and close with `};`. No `void onXxxEvent(...)`
boilerplate.

**The head carries the data, `WHEN_` carries the kind.** One kind of event, and
the parameters are enough:

```cpp
IJoystick(I2, float x, float y)  { drive(x, y); };
IHorizontalSlider(I1, float v)   { analogWrite(PWM, v); };
IVerticalSlider(I4, float v)     { … };
ISwitch(I3, bool on)             { digitalWrite(RELAY, on); };
ISegmentedSwitch(I5, int index)  { mode = index; };
ISignal(I6, const char* text)    { … };
```

Several kinds, and `WHEN_` sorts them — which is what stops a sketch from
growing a staircase of `if`:

| Block | `WHEN_` clauses |
|---|---|
| `ISimpleButton(I0)` | `WHEN_PRESSED`, `WHEN_RELEASED`, `WHEN_LONG_PRESSED` |
| `IAdvancedButton(I0)` | same |
| `IEmergencyButton(I0)` | `WHEN_TRIGGERED`, `WHEN_RESET` |
| `IDirectionPad(I0)` | `WHEN_UP`…`WHEN_CENTER`, `WHEN_*_LONG`, `WHEN_*_RELEASE`, `WHEN_RELEASED_ANY`, `WHEN_PAD_PRESSED(key)`, `WHEN_PAD_RELEASED(key)`, `WHEN_PAD_LONG_PRESSED(key)` |

```cpp
IDirectionPad(I7) {
    WHEN_UP           { forward(); }        // one key, one gesture
    WHEN_UP_LONG      { faster(); }
    WHEN_RELEASED_ANY { stop(); }
    WHEN_PAD_PRESSED(key) { … }             // every key, same treatment
};
```

You write the type of what you receive: `float`, `bool`, any integer type, or
`const char*`. The board is the only place that knows what an address holds.

Several blocks may watch one address. And a catch-all, for logging or a board
that routes addresses itself:

```cpp
void onSignalWritten(const SignalEvent& e) {
    Serial.print("I"); Serial.println(e.address);
}
```

---

## Sending

```cpp
InstantIoT.write(I0, readTemperature());   // float
InstantIoT.write(I1, digitalRead(PIN));    // bool
InstantIoT.write(I2, rpm);                 // int
InstantIoT.write(I3, "OK");                // text, 48 characters max
```

`InstantIoT.write` works from **anywhere** — any function, any block, any
`WHEN_` clause. Answering a gesture with a write is the normal case. Two
caveats, and they are real: before `begin()` the call does nothing and says so
once; and never from an interrupt — set a flag, write in `loop()`.

**You may call `write()` on every pass of `loop()`.** The library applies the
platform ceiling, the same frames-per-second the server's fuse enforces, so a
sketch without a `delay()` cannot get itself disconnected for flooding.
`write()` returns `false` when the ceiling swallowed the call — not an error,
and most sketches ignore it.

Two things the compiler catches. `write(IO, x)` — capital O instead of zero —
does not compile, and it even guesses right:

```
error: 'IO' was not declared in this scope; did you mean 'I9'?
```

And declaring your own `I1` gives a redefinition error rather than a puzzling
message about a string literal.

### A setpoint survives a reboot

It is a **state**, not a gesture — so the server keeps it and replays it the
moment the board reconnects. A pump asked to run at 19 °C last Tuesday is asked
again on Wednesday morning, with nobody's phone open and nothing saved in
EEPROM. The `ISignal(I5, float)` block simply runs again on connect.

A `ISimpleButton(I5)` would **not** run: nobody pressed. Delivering it would
invent a gesture, and the sketch would light a lamp nobody asked for. The rule
belongs to the block, not to the signal's settings.

---

## Timing

`InstantTimer` comes with the same header. Prefer it to a hand-copied
`millis() - last >= …`, and never use `delay()` — it stops reading incoming
frames too.

```cpp
InstantTimer timers;
void publish() { InstantIoT.write(I0, readSensor()); }

void setup() { …; timers.every(1000, publish); }
void loop()  { InstantIoT.loop(); timers.run(); }
```

`every(ms, fn)`, `once(ms, fn)`, `times(ms, fn, n)`, plus `enable`, `disable`,
`cancel`, `changeInterval`.

---

## Examples

**File → Examples → InstantIoT**

| Folder | What it shows |
|---|---|
| `connection/` | `OwnServer`, `TheCloud` — the shortest sketch that connects |
| `controls/` | the nine control blocks: buttons, switch, sliders, joystick, D-pad, segmented |
| `measurements/` | `UltrasonicLevel` (HC-SR04), `AnalogTemperature` (LM35, no library to install), `TextAndState` |
| `complete/` | `Thermostat` (both directions, with replay), `Dashboard`, `Diagnostic` |

The measurement examples are named after the **sensor**, not the widget —
because the widget is not the sketch's business any more. Each one says which
widget to put on which address in the app, and that another would do just as
well.

---

## When it does not connect

The board says why, on the serial monitor, without recompiling:

```
[InstantIoT] WiFi refused — reason 15 : key refused by the access point (wrong password)
```

ESP32 only for now — WiFiS3 on the Uno R4 has no event API. It speaks once per
distinct reason, never once per retry. `#define INSTANTIOT_QUIET 1` silences
it.

For the full trace — every step of Wi-Fi, TLS and handshake:

```cpp
#define INSTANTIOT_DEBUG 1
#include <InstantIoT.h>
```

Failing on the first attempt is normal: at boot the router is not always ready.
`loop()` keeps retrying with a growing backoff — call it unconditionally, even
when `begin()` returned `false`.

### When frames arrive but nothing happens

A frame the board cannot read is counted, and said once. Publish the counter
and you can watch it from the app instead of from a serial cable:

```cpp
InstantIoT.write(I9, InstantIoT.ignoredFrames());
```

Zero is the normal answer. A counter that climbs means the server or the app
is sending something this version does not understand — the kind of fault
that otherwise looks exactly like "my button does nothing".

---

## Settings

All of them go **before** the include.

```cpp
#define INSTANTIOT_DEBUG                    1      // full trace (default 0)
#define INSTANTIOT_QUIET                    1      // no connection diagnostics at all
#define INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS  30000  // default 15000
#define INSTANTIOT_DEFAULT_SIGNAL_RATE      10     // frames/s ceiling, burst of 2x (mirrors the relay fuse)
#define INSTANTIOT_CLOUD_HOST               "staging.example"
#define INSTANT_AP_PORT                     8080
```

Per-destination, at the call site: `.heartbeatEvery(20000)` (held to 1 s … 48 s), `.at(host, port)`.

---

## Bluetooth and Serial

The code is in the tree, it compiles, and the wire protocol is the same — but
the mobile app does not expose a selector for these yet, so an end-to-end
connection is not possible today. They are kept so the work is not lost.

| Link | Extra setup |
|---|---|
| `BluetoothLink(name)` — Classic, ESP32 | needs the `huge_app` partition scheme: Bluetooth Classic plus Wi-Fi does not fit in the default 1.3 MB |
| `BLELink(name)` — ESP32 | install [`NimBLE-Arduino`](https://github.com/h2zero/NimBLE-Arduino), **and `#include <NimBLEDevice.h>` before `<InstantIoT.h>`** — Arduino only finds a library it sees included |
| `SerialLink(rx, tx)` — AVR, ESP8266 | an external Bluetooth-Serial module (HC-05 / HC-06) wired to the board |

---

## Under the hood

Static allocation end to end — no heap, no `String`, no JSON. A signal frame is
14 bytes for a float. `ARCHITECTURE.md` walks the protocol byte by byte, and
`test/host/run.sh` runs the whole bench in about a second, with no board.

---

## Links

- 🌐 [instantiot.io](https://instantiot.io)
- 📱 [InstantIoT on Google Play](https://play.google.com/store/apps/details?id=com.jeanloickdt.instantiot)
- 💬 [Community & Support](mailto:bonjour@jeanloickdt.com)

---

## License

MIT License — Copyright (c) 2026 Djoufack Tsobeng Jean Loick — see [`LICENSE`](LICENSE).
