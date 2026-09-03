#pragma once
/**
 * ============================================================
 * ⚡ InstantIoT.h — the sketch's one object
 * ============================================================
 *
 * ## What it replaces
 *
 * Every sketch used to declare its own facade as a global:
 *
 *     InstantIoTWiFiAP instant("MyBoard", "12345678");
 *     void setup() { instant.begin(); }
 *     void loop()  { instant.loop();  }
 *
 * And `instant` was only reachable because the sketch had thought to
 * declare it OUTSIDE `setup()`. Declared inside, no function and no
 * `ISignal` block could write any more — and nothing in the library said
 * so. The name also changed with the link, so an example could not be
 * copied from one mode to another.
 *
 *     void setup() { InstantIoT.begin(link); }
 *     void loop()  { InstantIoT.loop();      }
 *
 * ## Callable from where?
 *
 * Everywhere, and that is the point. From `loop()`, from a function of
 * your own, from an `ISignal` or `ISimpleButton` block — answering a
 * gesture with a write is the normal case, and the read and write paths
 * do not share a buffer (`_rxBuffer` on one side, `_txBuffer` on the
 * other).
 *
 * Two caveats, and both are real:
 *
 * **Before `begin()`** there is no link. The call does nothing and says
 * so once. It dereferences nothing: this object's fields are
 * constant-initialised, so they are valid even while another translation
 * unit's globals are still being constructed. A plain global would not
 * guarantee that, which is why this one has no constructor.
 *
 * **From an interrupt**, no — writing to a socket from an ISR is unsafe
 * with any library. The Arduino rule does not change: the interrupt sets
 * a flag, `loop()` writes.
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

/**
 * The DSL comes along.
 *
 * No header used to pull `InstantIoTWhen.hpp` in: a sketch only had
 * `ISignal` if it thought to include it itself, and none of the examples
 * did — they stopped compiling the day the DSL was rebuilt on signals.
 * One `#include <InstantIoT.h>` must be enough for everything a sketch
 * writes.
 */
#include "utils/InstantIoTWhen.hpp"

/** The links and destinations `begin()` accepts. */
#include "Links.hpp"

/** Timers — `timers.every(1000, publish)` rather than a hand-rolled `millis()`. */
#include "utils/InstantIoTTimer.hpp"

namespace iiot {

/**
 * The facade. One instance, named `InstantIoT`, declared below.
 *
 * It does not do the work: it holds the core and hands over. The core
 * only exists from `begin()` onwards, because that is the only moment we
 * know which link the board speaks through.
 */
class Facade {
public:

    /**
     * Open a direct link — the app is at the other end of the wire.
     *
     *     InstantIoT.begin(AccessPoint("MyBoard", "12345678"));
     *     InstantIoT.begin(BluetoothLink("MyBoard"));
     *
     * The link passed in is a description: it does not outlive the
     * statement, and the facade keeps the transport it yields.
     *
     * The `decltype` is not decoration: without it this overload would
     * also take raw transports, which have no `transport()`, and the
     * error would talk about templates instead of saying what to fix.
     */
    template <class Link>
    auto begin(const Link& link) -> decltype(link.transport(), bool()) {
        return begin(link.transport());
    }

    /**
     * Open a link towards a destination.
     *
     *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
     *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
     *
     * The link does not know what the far end is, and the destination
     * does not know how it is reached. That is what will let one more
     * link — Ethernet, one day — force no destination to change.
     */
    template <class Link, class Dest>
    bool begin(const Link& link, const Dest& dest) {
        const bool first = (_core == nullptr);
        const bool opened = begin(link.transportTo(dest));
        // The heartbeat is plumbed on both layers: the transport
        // announces it to the server at the handshake, the core emits it.
        // Not on a second `begin()`, which rebuilds nothing.
        if (first) setHeartbeat(dest.heartbeatMs);
        return opened;
    }

    /**
     * Open a link that is already built.
     *
     * The escape hatch: a transport of your own, or one of the library's
     * instantiated by hand. It must live as long as the program does —
     * a global or a `static`, never a temporary.
     *
     * @return true if the link opened.
     */
    bool begin(ITransport& link) {
        // The `static` is built on the first pass, with THIS link, and
        // never rebuilt. A second `begin()` would therefore not rebuild
        // the core: it would merely let you believe it had. Say so.
        if (_core) {
            IIOT_LOG("[InstantIoT] begin() was already called — the second is ignored");
            return _core->connected();
        }
        static InstantIoTCoreBase core(link);
        _core = &core;
        return core.begin();
    }

    /** Call from `loop()`, unconditionally. */
    void loop() { if (_core) _core->loop(); }

    bool connected() { return _core && _core->connected(); }

    void setHeartbeat(uint32_t intervalMs) {
        if (_core) _core->setHeartbeat(intervalMs);
        else tooEarly();
    }

    void setSignalRateLimit(uint16_t framesPerSecond) {
        if (_core) _core->setSignalRateLimit(framesPerSecond);
        else tooEarly();
    }

    /**
     * Write a value to a signal.
     *
     * The template only forwards: the core carries the overloads,
     * `double` and `unsigned long` included, so that
     * `write(I0, analogRead(A0) * 3.3 / 4095.0)` compiles.
     *
     * @return false when nothing left — no link, or the frames-per-second
     *         ceiling swallowed the call. Neither is a mistake by the
     *         sketch, and neither deserves a reboot.
     */
    template <class V>
    bool write(SignalRef sig, V value) {
        if (!_core) return tooEarly();
        return _core->write(sig, value);
    }

    /**
     * How many frames arrived that the board could not read.
     *
     * Zero is the normal answer. A counter that climbs means the server
     * or the app is sending something this version does not understand —
     * the kind of fault that, without this number, looks exactly like
     * "my button does nothing".
     */
    uint32_t ignoredFrames() const { return _core ? _core->ignoredFrames() : 0; }

    /**
     * The board's configuration.
     *
     * It belongs to the core, which only exists after `begin()`. Before
     * that there is nothing to return and no reference to invent: this
     * empty store absorbs the writes so the call still means something
     * too early, and the log says they will not apply.
     */
    DeviceConfig& config() {
        if (_core) return _core->config();
        tooEarly();
        static DeviceConfig noEffect;
        return noEffect;
    }

private:
    InstantIoTCoreBase* _core = nullptr;

    /**
     * The one place that answers "not yet".
     *
     * Once, and not on every pass: called from `loop()`, a chatty log
     * drowns the useful line and slows things enough to change the
     * symptom being observed.
     */
    bool _alreadyWarned = false;
    bool tooEarly() {
        if (!_alreadyWarned) {
            _alreadyWarned = true;
            IIOT_LOG("[InstantIoT] call before begin() — with no link, nothing leaves");
        }
        return false;
    }
};

}  // namespace iiot

/**
 * The one object.
 *
 * It carries the name the namespace used to carry, now `iiot` to make
 * room: in C++ an object and a namespace cannot share a name, and it is
 * the sketch that needs the readable one.
 */
extern iiot::Facade InstantIoT;

/**
 * The names a sketch writes, in scope without declaring anything.
 *
 * Without these lines every sketch had to start with
 * `using namespace iiot;` — a formula to copy without understanding, in
 * a library whose whole point is that there should be none.
 *
 * Named `using` declarations, not a `using namespace`: only these names
 * come out, and the internals — `Facade`, `InstantIoTCoreBase`,
 * `SignalRegistrar`, the transports — stay tidied away. Whoever wants one
 * writes `iiot::` and knows they are going down a floor.
 *
 * They are unconditional: a link a board does not have still exists, as a
 * shell, so that the error message is the real one.
 */
using iiot::AccessPoint;
using iiot::WiFiLink;
using iiot::BluetoothLink;
using iiot::BLELink;
using iiot::SerialLink;
using iiot::EthernetLink;

using iiot::Cloud;
using iiot::MyServer;
using iiot::SecureDestination;
using iiot::PlainDestination;

/** Direction-pad keys — `WHEN_PAD_PRESSED(k) { if (k == DPadButton::A) … }` */
using iiot::DPadButton;
