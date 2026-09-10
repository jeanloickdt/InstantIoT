/*
 * What the board does with a wire that misbehaves, pinned on a laptop.
 *
 * A relay in trouble, a hostile intermediary or a gateway in a loop can all
 * send more than a board wants. This bench checks that one `loop()` reads a
 * bounded amount and hands the turn back, that a burst of noise does not
 * lose the frame that follows it, and that the legacy encoder refuses a
 * name that does not fit instead of writing past its buffer.
 */

#include <Arduino.h>
#include <stdio.h>

#include "../../src/core/BinaryCodec.hpp"
#include "../../src/core/SignalEvents.hpp"
#include "../../src/core/InstantIoTCore.hpp"

using namespace iiot;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

static int signalsSeen = 0;
void onSignalWritten(const SignalEvent& e) { (void)e; signalsSeen++; }

/** A transport fed by hand: whatever is queued is what the board reads. */
struct ScriptedTransport : ITransport {
    static const size_t CAP = 64 * 1024;
    uint8_t queue[CAP];
    size_t  head = 0, tail = 0;
    int     reads = 0;

    void feed(const uint8_t* b, size_t n) { for (size_t i = 0; i < n && tail < CAP; i++) queue[tail++] = b[i]; }
    void noise(size_t n, uint8_t byte = 0x55) { for (size_t i = 0; i < n && tail < CAP; i++) queue[tail++] = byte; }
    size_t pending() const { return tail - head; }

    bool   begin() override     { return true; }
    void   poll() override      {}
    bool   connected() override { return true; }
    int    available() override { return (int)pending(); }
    int    read(uint8_t* out, size_t len) override {
        reads++;
        size_t n = pending() < len ? pending() : len;
        memcpy(out, queue + head, n);
        head += n;
        return (int)n;
    }
    size_t write(const uint8_t*, size_t len) override { return len; }
};

static ScriptedTransport wire;

struct TestCore : InstantIoTCoreBase {
    TestCore() : InstantIoTCoreBase(wire) {}
    using InstantIoTCoreBase::readLoop;
};
static TestCore core;

int main() {
    BinaryCodec codec;
    uint8_t frame[64];
    float v = 23.4f;
    size_t frameLen = codec.encodeSignal(frame, sizeof(frame), 5, SIGNAL_TAG_FLOAT, (const uint8_t*)&v, 4);
    ok(frameLen > 0, "the golden frame builds");

    section("One pass reads a bounded amount, then hands the turn back");
    wire.noise(20000);
    core.readLoop();
    ok(wire.pending() > 0, "a peer that never stops talking does not own loop()");
    ok(20000 - wire.pending() <= INSTANT_RX_BUFFER_SIZE, "one pass reads at most one buffer's worth");
    size_t passes = 1;
    while (wire.pending() > 0 && passes < 10000) { core.readLoop(); passes++; }
    ok(wire.pending() == 0, "the noise is eventually drained");
    ok(signalsSeen == 0, "noise produced no signal");

    section("A frame after a burst of noise is still read");
    wire.noise(300, 0x55);
    wire.feed(frame, frameLen);
    while (wire.pending() > 0) core.readLoop();
    ok(signalsSeen == 1, "the frame that follows the noise reaches the sketch");

    section("Noise that looks like a start byte does not swallow the frame");
    signalsSeen = 0;
    wire.noise(100, 0xAA);
    wire.feed(frame, frameLen);
    while (wire.pending() > 0) core.readLoop();
    ok(signalsSeen == 1, "a run of 0xAA still lets the real frame through");

    section("More valid frames than the buffer holds, in one pass");
    signalsSeen = 0;
    size_t many = (INSTANT_RX_BUFFER_SIZE / frameLen) + 3;
    for (size_t i = 0; i < many; i++) wire.feed(frame, frameLen);
    while (wire.pending() > 0) core.readLoop();
    ok((size_t)signalsSeen == many, "every frame arrives, none lost to an overflow reset");

    section("The legacy encoder refuses what does not fit");
    char longName[300];
    memset(longName, 'x', sizeof(longName) - 1);
    longName[sizeof(longName) - 1] = '\0';
    uint8_t out[512];
    ok(codec.encode(out, sizeof(out), "dev", longName, 0x01, 0x00) == 0,
       "a widget name longer than a byte is refused, not truncated and overrun");
    char name200[201]; memset(name200, 'w', 200); name200[200] = '\0';
    char dev200[201];  memset(dev200,  'd', 200); dev200[200]  = '\0';
    ok(codec.encode(out, sizeof(out), dev200, name200, 0x01, 0x00) == 0,
       "two names that fit a byte each but not the body together are refused");
    ok(codec.encode(out, sizeof(out), "dev1", "btn1", 0x01, 0x00) > 0,
       "an ordinary frame still builds");

    printf("\n%d checks, %d failure%s\n", checks, failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
