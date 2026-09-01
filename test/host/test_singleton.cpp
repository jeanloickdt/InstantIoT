/*
 * The one object — what it promises, and the places it lies.
 *
 * What a sketch expects from `InstantIoT` fits in one sentence: **it can
 * be called from anywhere**. Several of those "anywhere"s cannot be
 * checked by reading:
 *
 *   - before `begin()`, when there is no link yet;
 *   - from an `ISignal` block, that is, while the core is reading a frame;
 *   - twice `begin()`, which a reworked sketch always ends up doing.
 */

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../../src/InstantIoT.h"
#include "../../src/utils/InstantIoTWhen.hpp"

using namespace iiot;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

// ── A paper link ──────────────────────────────────────────────────────────
//
// It keeps what goes out and returns what was put in: enough to watch a
// write leave, and a frame arrive.

struct PaperLink : ITransport {
    uint8_t sent[256];  size_t sortiLen = 0;
    uint8_t toRead[256];  size_t toReadLen = 0, toReadPos = 0;
    bool    opened = false;

    bool begin() override      { opened = true; return true; }
    void poll() override       {}
    bool connected() override  { return opened; }
    int  available() override  { return (int)(toReadLen - toReadPos); }

    int read(uint8_t* buf, size_t len) override {
        size_t n = toReadLen - toReadPos;
        if (n > len) n = len;
        memcpy(buf, toRead + toReadPos, n);
        toReadPos += n;
        return (int)n;
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (sortiLen + len > sizeof(sent)) return 0;
        memcpy(sent + sortiLen, buf, len);
        sortiLen += len;
        return len;
    }

    void deposit(const uint8_t* trame, size_t len) {
        memcpy(toRead, trame, len);
        toReadLen = len;
        toReadPos = 0;
    }
    void forget() { sortiLen = 0; }
};

static PaperLink link;

// ── The block that answers ────────────────────────────────────────────────
//
// The case we want to be sure of: writing FROM a block, that is, while the
// core is processing a received frame.

static int i5Received = 0;
ISignal(I5, float setpoint) {
    i5Received++;
    InstantIoT.write(I6, setpoint * 2.0f);
};

// ── Reading a signal frame ────────────────────────────────────────────────

struct Read { bool ok = false; uint8_t addr = 0, tag = 0; float value = 0; };

static Read readFrame(const uint8_t* trame, size_t len) {
    Read r;
    const uint8_t* payload = nullptr;
    size_t chargeLen = 0;
    bool restore = false;
    r.ok = BinaryCodec::decodeSignal(trame, len, r.addr, r.tag, payload, chargeLen, restore);
    if (r.ok && chargeLen == 4) memcpy(&r.value, payload, 4);
    return r;
}

int main() {
    section("Before begin(): nothing, and above all no crash");
    {
        // No link has been opened. A sketch writing here has made a
        // mistake — the library must say so, not reboot it.
        ok(!InstantIoT.connected(), "nobody is connected");
        ok(!InstantIoT.write(I0, 1.0f), "a write does not leave");
        ok(!InstantIoT.write(I0, 42), "…whatever the type");
        ok(!InstantIoT.write(I0, "bonjour"), "…text included");
        InstantIoT.loop();          // ne doit rien faire, et surtout revenir
        InstantIoT.setHeartbeat(5000);
        InstantIoT.setSignalRateLimit(30);
        ok(true, "loop() and the settings pass with no link");
    }

    section("After begin(): the write reaches the link");
    {
        ok(InstantIoT.begin(link), "the link opens");
        ok(InstantIoT.connected(), "…and reports itself connected");

        link.forget();
        ok(InstantIoT.write(I5, 23.4f), "a value leaves");
        Read r = readFrame(link.sent, link.sortiLen);
        ok(r.ok, "what left really is a signal frame");
        ok(r.addr == 5, "at the right address");
        ok(r.value > 23.39f && r.value < 23.41f, "with the right value");
    }

    section("Twice begin(): the second builds no second core");
    {
        PaperLink other;
        InstantIoT.begin(other);
        link.forget();
        other.forget();
        InstantIoT.write(I5, 1.0f);
        ok(link.sortiLen > 0, "the first link is still in use");
        ok(other.sortiLen == 0, "the second received nothing — begin() does not replay");
    }

    section("A link that opens badly keeps trying");
    {
        // The field case: WiFi is not up yet, or the server does not
        // answer. `begin()` returns false — and that is normal. What must
        // NOT happen is the board giving up: the reconnection with backoff
        // lives in the transport's `poll()`, and `loop()` is the only
        // caller.
        struct FailingLink : ITransport {
            int  polls = 0;
            bool begin() override      { return false; }   // nothing answers
            void poll() override       { polls++; }
            bool connected() override  { return false; }
            int  available() override  { return 0; }
            int  read(uint8_t*, size_t) override { return -1; }
            size_t write(const uint8_t*, size_t) override { return 0; }
        };
        // Aim at the core and not the facade: the facade's core is a
        // function-local `static`, so UNIQUE. A second `Facade` reuses the
        // first one's — which is what makes it a singleton for real, and
        // what makes a second instance useless for this test.
        static FailingLink mute;
        InstantIoTCoreBase core(mute);
        ok(!core.begin(), "begin() says plainly that it failed");

        core.loop();
        core.loop();
        core.loop();
        ok(mute.polls == 3,
           "…and loop() keeps the transport running, otherwise the "
           "reconnection with backoff never happens and the board is dead "
           "until reset");
    }

    section("A frame the board cannot handle does not evaporate");
    {
        // The case that nearly went unnoticed: the app was sending EVENT
        // frames (TYPE 0x21) the board no longer decodes. They vanished
        // without a word, and the sketch had no way to know anything had
        // arrived.
        //
        // A frame we cannot handle is INFORMATION: either the other end
        // speaks a language we have stopped understanding, or it is noise
        // on the wire. Both can be diagnosed; neither should be silent.
        BinaryCodec codec;
        uint8_t event[64];
        // A frame shaped exactly like a signal, but TYPE 0x21.
        size_t n = codec.encodeSignal(event, sizeof(event), 5,
                                      SIGNAL_TAG_FLOAT, nullptr, 0);
        ok(n > 0, "the frame is built");
        event[7] = 0x21;                    // TYPE_SIGNAL → TYPE_EVENT
        event[n - 1] = crc8(event + 4, n - 5);

        uint32_t before = InstantIoT.ignoredFrames();
        link.deposit(event, n);
        InstantIoT.loop();
        ok(InstantIoT.ignoredFrames() == before + 1,
           "the board counts the frame it could not read");
    }

    section("From a block: writing while reading");
    {
        // Build the incoming frame with our own encoder, then drop it into
        // the link: the core will read it on the next loop().
        uint8_t incoming[64];
        BinaryCodec codec;
        uint8_t payload[4];
        float setpoint = 10.0f;
        memcpy(payload, &setpoint, 4);
        size_t n = codec.encodeSignal(incoming, sizeof(incoming), 5, SIGNAL_TAG_FLOAT, payload, 4);
        ok(n > 0, "the incoming frame encodes");

        link.deposit(incoming, n);
        link.forget();
        i5Received = 0;

        InstantIoT.loop();

        ok(i5Received == 1, "the ISignal block really was called");
        Read r = readFrame(link.sent, link.sortiLen);
        ok(r.ok, "and the write made FROM the block went out");
        ok(r.addr == 6, "at the address the block asked for");
        ok(r.value > 19.9f && r.value < 20.1f, "with the value computed inside the block");
    }

    printf(failures ? "\n%d FAILURE(S) of %d\n" : "\nall pass (%d/%d)\n",
           failures ? failures : checks, checks);
    return failures ? 1 : 0;
}
