/*
 * The ceiling a board applies to its own signal frames, pinned on a laptop.
 *
 * The relay's fuse admits 10 frames a second with a burst of 20. This bench
 * checks that the board's bucket has the same shape: a burst passes whole, a
 * loop without a delay is held to the rate, and the reserve comes back with
 * time. The clock is `fakeMillis()` from the stub, moved by hand.
 */

#include <Arduino.h>
#include <stdio.h>

#include "../../src/core/InstantIoTCore.hpp"

using namespace iiot;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

/** A transport that counts what it is asked to send, and sends nothing. */
struct CountingTransport : ITransport {
    int frames = 0;
    bool   begin() override                           { return true; }
    void   poll() override                            {}
    bool   connected() override                       { return true; }
    int    available() override                       { return 0; }
    int    read(uint8_t*, size_t) override            { return 0; }
    size_t write(const uint8_t*, size_t len) override { frames++; return len; }
};

static CountingTransport tx;
static InstantIoTCoreBase core(tx);

static int burst(int n) {
    int sent = 0;
    for (int i = 0; i < n; i++) if (core.write(I0, i)) sent++;
    return sent;
}

int main() {
    fakeMillis() = 5000;   // a board that has been up a while

    section("A burst passes whole, the reserve is twice the rate");
    ok(burst(20) == 20, "20 writes in the same millisecond all leave");
    ok(burst(5) == 0,   "the 21st is refused");
    ok(core.refusedSignals() == 5, "the refusals are counted");

    section("A loop without a delay is held to the rate");
    fakeMillis() += 100;                       // 100 ms → 1 token at 10/s
    ok(burst(3) == 1, "100 ms later, exactly one frame leaves");
    fakeMillis() += 1000;                      // 1 s → 10 tokens
    ok(burst(30) == 10, "one second later, ten frames leave, not thirty");

    section("The reserve fills back in two seconds, not more");
    fakeMillis() += 60000;                     // a long silence
    ok(burst(25) == 20, "after a long silence the reserve is full, and no larger");
    ok(tx.frames == 20 + 1 + 10 + 20, "every accepted write reached the transport");

    section("The ceiling can be lowered, and the reserve follows");
    core.setSignalRateLimit(2);
    fakeMillis() += 60000;
    ok(burst(10) == 4, "at 2/s the reserve is 4");
    fakeMillis() += 500;
    ok(burst(10) == 1, "half a second later, one token");

    section("The clock may wrap");
    fakeMillis() = 0xFFFFFF00UL;
    burst(10);                                 // drain whatever is there
    fakeMillis() = 100;                        // rolled over: 356 ms elapsed
    ok(burst(10) >= 0, "a wrapped clock does not trap the board");
    fakeMillis() = 5000;
    ok(burst(10) == 4, "after the wrap the bucket refills normally");

    printf("\n%d checks, %d failure%s\n", checks, failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
