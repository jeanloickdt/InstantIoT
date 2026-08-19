/*
 * The receive half of the value path, pinned on a laptop.
 *
 * What these tests are really defending is the agreement with the server. The
 * golden frames below were produced from the protocol description, not from
 * this code — so if either side ever drifts, one of them stops matching bytes
 * the other never saw.
 */

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "../../src/core/BinaryCodec.hpp"
#include "../../src/core/SignalEvents.hpp"
#include "../../src/core/InstantIoTCore.hpp"
#include "../../src/utils/InstantIoTWhen.hpp"

using namespace InstantIoT;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void okNear(float got, float want, const char* what) {
    ok(fabsf(got - want) < 0.0005f, what);
    if (fabsf(got - want) >= 0.0005f) printf("    got %f, want %f\n", got, want);
}
static void section(const char* name) { printf("── %s\n", name); }

// ── Les trames d'or ───────────────────────────────────────────────────────
// Byte for byte what the server's SignalFrame.build produces.

static const uint8_t GOLD_FLOAT_23_4_AT_I5[] = {
    0xAA, 0x01, 0x09, 0x00, 0x00, 0x01, 0x05, 0x20, 0x03, 0x33, 0x33, 0xBB, 0x41, 0xF3
};
static const uint8_t GOLD_BOOL_TRUE_AT_I0[] = {
    0xAA, 0x01, 0x06, 0x00, 0x00, 0x01, 0x00, 0x20, 0x01, 0x01, 0x33
};
static const uint8_t GOLD_INT_16777217_AT_I7[] = {
    0xAA, 0x01, 0x09, 0x00, 0x00, 0x01, 0x07, 0x20, 0x02, 0x01, 0x00, 0x00, 0x01, 0xEE
};

struct Parsed {
    bool        recognised = false;
    uint8_t     address    = 0xFF;
    SignalValue value;
    char        text[49]   = {0};
};

static Parsed parse(const uint8_t* frame, size_t len) {
    Parsed p;
    uint8_t tag = 0;
    const uint8_t* payload = nullptr;
    size_t payloadLen = 0;
    if (!BinaryCodec::decodeSignal(frame, len, p.address, tag, payload, payloadLen)) return p;
    p.recognised = decodeSignalValue(tag, payload, payloadLen, p.value, p.text, sizeof(p.text));
    return p;
}

// ── Les blocs ISignal, tels qu'un sketch les écrit ────────────────────────

static int   i5Calls = 0;   static float i5Last = 0;
static int   i0Calls = 0;   static bool  i0Last = false;
static int   i5CallsSecond = 0;

ISignal(I5, float target) {
    i5Calls++;
    i5Last = target;
};

ISignal(I5, float target) {
    (void)target;
    i5CallsSecond++;
};

ISignal(I0, bool on) {
    i0Calls++;
    i0Last = on;
};

// A widget handler, to prove the new branch did not eat the old path.
static int btnCalls = 0;
ISimpleButton("btn1") {
    WHEN_PRESSED { btnCalls++; }
};

// The capture the example sketch uses for a text signal — here mostly so the
// macro expansion for a pointer type is compiled, not only the scalar ones.
static char i9Last[49] = {0};
ISignal(I9, const char* mode) {
    strncpy(i9Last, mode, sizeof(i9Last) - 1);
};

static int weakCalls = 0;
void onSignalWritten(const SignalEvent& e) { (void)e; weakCalls++; }

// ── Le vrai coeur, avec un transport qui ne transporte rien ───────────────
//
// The routing is tested through InstantIoTCoreBase::processFrame itself
// rather than a copy of it, because a copy would keep passing on the day the
// real one changes.

struct NullTransport : ITransport {
    bool   begin() override                        { return true; }
    void   poll() override                         {}
    bool   connected() override                    { return true; }
    int    available() override                    { return 0; }
    int    read(uint8_t*, size_t) override         { return 0; }
    size_t write(const uint8_t*, size_t len) override { return len; }
};

static NullTransport nullTransport;

struct TestCore : InstantIoT::InstantIoTCoreBase {
    TestCore() : InstantIoTCoreBase(nullTransport) {}
    using InstantIoTCoreBase::processFrame;
};

static TestCore core;
static bool route(const uint8_t* frame, size_t len) {
    int before = weakCalls;
    core.processFrame(frame, len);
    return weakCalls > before;
}

// ── Ce que l'utilisateur ecrit vraiment ───────────────────────────────────
//
// Ce bloc ne s'execute pas : il doit COMPILER. C'est le seul test possible
// pour une ambiguite de surcharge, et il aurait attrape le defaut qui a
// bloque un vrai croquis — `3.3 / 4095.0` vaut un double, et sans surcharge
// dediee le compilateur voit trois candidats a egalite.
struct NullTx2 : ITransport {
    bool   begin() override                           { return true; }
    void   poll() override                            {}
    bool   connected() override                       { return false; }
    int    available() override                       { return 0; }
    int    read(uint8_t*, size_t) override            { return 0; }
    size_t write(const uint8_t*, size_t len) override { return len; }
};
static NullTx2 tx2;
static InstantIoTCoreBase writeCompiles(tx2);

static void everyNaturalWriteCompiles() {
    int raw = 2048;
    writeCompiles.write(I0, raw * (3.3 / 4095.0) * 100.0);  // double — le cas reel
    writeCompiles.write(I1, 23.4);                          // litteral decimal
    writeCompiles.write(I2, 23.4f);
    writeCompiles.write(I3, true);
    writeCompiles.write(I4, 42);
    writeCompiles.write(I5, 42L);                           // long
    writeCompiles.write(I6, millis());                      // unsigned long
    writeCompiles.write(I7, (unsigned int)7);
    writeCompiles.write(I8, (short)7);
    writeCompiles.write(I9, (uint8_t)7);
    writeCompiles.write(I10, "OK");

    // Toute la bibliotheque mathematique rend des `double`. Une courbe de
    // test, une conversion, une moyenne : c'est le cas le PLUS courant, pas
    // un cas limite.
    writeCompiles.write(I11, sin(millis() / 1000.0));
    writeCompiles.write(I12, sqrt(2.0));
    writeCompiles.write(I13, (raw + 0.5) / 2);
}

int main() {
    (void)&everyNaturalWriteCompiles;

    BinaryCodec codec;

    section("La trame que le serveur envoie vraiment");
    {
        Parsed p = parse(GOLD_FLOAT_23_4_AT_I5, sizeof(GOLD_FLOAT_23_4_AT_I5));
        ok(p.recognised, "a float setpoint from the server is recognised");
        ok(p.address == 5, "…at the address the server addressed it to");
        okNear(p.value, 23.4f, "…with the value it sent");
        ok(sizeof(GOLD_FLOAT_23_4_AT_I5) == 14, "a float setpoint is 14 bytes, as costed");
    }
    {
        Parsed p = parse(GOLD_BOOL_TRUE_AT_I0, sizeof(GOLD_BOOL_TRUE_AT_I0));
        ok(p.recognised && p.address == 0,
           "I0 survives — the address is a byte, and readString would have read it as a length");
        ok((bool)p.value, "a bool setpoint reads as true");
    }
    {
        // 2^24 + 1: the first integer a float cannot hold.
        Parsed p = parse(GOLD_INT_16777217_AT_I7, sizeof(GOLD_INT_16777217_AT_I7));
        long asLong = p.value;
        ok(p.recognised && p.address == 7, "an int setpoint is recognised");
        ok(asLong == 16777217L, "…and keeps every bit: a counter is not a rounded counter");
    }

    section("Ce qui n'est pas un signal n'est pas réclamé");
    {
        uint8_t frame[128];
        size_t n = codec.encode(frame, sizeof(frame), "dev1", "btn1",
                                TYPE_SIMPLEBUTTON, CMD_PRESS);
        ok(n > 0, "a widget frame still encodes");
        uint8_t a = 0, t = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        ok(!BinaryCodec::decodeSignal(frame, n, a, t, pl, pn),
           "…and the signal decoder leaves it alone, or every button press would vanish");

        DecodedMessage msg; uint8_t tc = 0, ec = 0;
        ok(codec.decode(frame, n, msg, tc, ec) && tc == TYPE_SIMPLEBUTTON,
           "…and it still decodes as the button it is");
    }
    {
        // The dangerous one: a widget frame shaped EXACTLY like a signal —
        // no device list, a one-character widget id. Only the TYPE byte tells
        // them apart, so only the TYPE check can save this press.
        uint8_t frame[128];
        size_t n = codec.encode(frame, sizeof(frame), nullptr, "A",
                                TYPE_SIMPLEBUTTON, CMD_PRESS);
        ok(n > 0 && frame[4] == 0x00 && frame[5] == 0x01,
           "the collision case is really built: DEV_COUNT 0, WID_LEN 1");
        uint8_t a = 0, t = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        ok(!BinaryCodec::decodeSignal(frame, n, a, t, pl, pn),
           "a one-letter widget id on a device-less frame is not a signal");
    }
    {
        uint8_t corrupted[sizeof(GOLD_FLOAT_23_4_AT_I5)];
        memcpy(corrupted, GOLD_FLOAT_23_4_AT_I5, sizeof(corrupted));
        corrupted[sizeof(corrupted) - 2] ^= 0xFF;   // one byte of the value
        uint8_t a = 0, t = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        ok(!BinaryCodec::decodeSignal(corrupted, sizeof(corrupted), a, t, pl, pn),
           "a corrupted value is refused, not applied — a setpoint acts on hardware");
    }
    {
        uint8_t truncated[sizeof(GOLD_FLOAT_23_4_AT_I5) - 3];
        memcpy(truncated, GOLD_FLOAT_23_4_AT_I5, sizeof(truncated));
        uint8_t a = 0, t = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        ok(!BinaryCodec::decodeSignal(truncated, sizeof(truncated), a, t, pl, pn),
           "a frame cut short by the transport is refused");
    }

    section("Aller-retour avec notre propre encodeur");
    {
        uint8_t frame[64];
        uint8_t payload[4];
        float v = -0.125f;
        memcpy(payload, &v, 4);
        size_t n = codec.encodeSignal(frame, sizeof(frame), 200, SIGNAL_TAG_FLOAT, payload, 4);
        Parsed p = parse(frame, n);
        ok(p.recognised && p.address == 200, "what we send, we can read back");
        okNear(p.value, -0.125f, "…including a negative value");
    }
    {
        uint8_t frame[64];
        const char* msg = "MAINTENANCE";
        size_t n = codec.encodeSignal(frame, sizeof(frame), 12, SIGNAL_TAG_STRING,
                                      (const uint8_t*)msg, strlen(msg));
        Parsed p = parse(frame, n);
        const char* got = p.value;
        ok(p.recognised && strcmp(got, "MAINTENANCE") == 0,
           "a text arrives terminated — the frame carries a length, not a NUL");
    }
    {
        // 55 characters where the receive buffer holds 48.
        char longText[56];
        memset(longText, 'x', 55); longText[55] = '\0';
        uint8_t frame[128];
        size_t n = codec.encodeSignal(frame, sizeof(frame), 13, SIGNAL_TAG_STRING,
                                      (const uint8_t*)longText, 55);
        ok(n > 0, "the frame was built");
        Parsed p = parse(frame, n);
        const char* got = p.value;
        ok(p.recognised && strlen(got) == 48,
           "a text longer than the buffer is cut, never written past it");
    }

    section("Les lectures d'une même valeur");
    {
        uint8_t frame[64];
        uint8_t payload[4];
        float v = 0.5f;
        memcpy(payload, &v, 4);
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_FLOAT, payload, 4);
        Parsed p = parse(frame, n);

        float  asFloat = p.value;
        int    asInt   = p.value;
        bool   asBool  = p.value;
        uint8_t asByte = p.value;

        okNear(asFloat, 0.5f, "read as a float, it is 0.5");
        ok(asInt == 0, "read as an int, it truncates — the sketch asked for an int");
        ok(asBool, "read as a bool, 0.5 is true and NOT the truncated 0");
        ok(asByte == 0, "read as a uint8_t, it compiles at all — that is the point of the template");
    }
    {
        uint8_t frame[64];
        uint8_t zero[4] = {0, 0, 0, 0};
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_FLOAT, zero, 4);
        Parsed p = parse(frame, n);
        ok(!(bool)p.value, "0.0 is false");
    }
    {
        // The one reading the numeric conversion cannot express: a text is
        // true when there is text. Without a bool conversion of its own, an
        // empty string and "MAINTENANCE" would both be false.
        uint8_t frame[64];
        const char* msg = "MAINTENANCE";
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_STRING,
                                      (const uint8_t*)msg, strlen(msg));
        ok((bool)parse(frame, n).value, "a text signal with text in it is true");

        n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_STRING, (const uint8_t*)"", 0);
        ok(!(bool)parse(frame, n).value, "…and an empty one is false");
    }
    {
        uint8_t frame[64];
        uint8_t two[1] = {2};   // anything non-zero
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_BOOL, two, 1);
        Parsed p = parse(frame, n);
        ok((bool)p.value, "a bool payload of 2 is still true");
    }

    section("Le desaccord entre la declaration et la capture");
    {
        // The server owns the type, the sketch owns the capture, and no
        // compiler sees both. The only mismatch that lies outright is a text
        // read as a number: it hands back 0 and looks like a real reading.
        uint8_t frame[64];
        const char* mode = "ECO";
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_STRING,
                                      (const uint8_t*)mode, strlen(mode));
        Parsed p = parse(frame, n);

        Serial.clear();
        float asFloat = p.value;
        ok(asFloat == 0.0f, "a text read as a number gives 0…");
        ok(Serial.saw("carries text"), "…and says so, which is the only warning there can be");

        Serial.clear();
        int asInt = p.value; (void)asInt;
        ok(Serial.saw("carries text"), "any numeric type, not just float");

        Serial.clear();
        const char* asText = p.value; (void)asText;
        ok(!Serial.saw("carries text") && !Serial.saw("not a text signal"),
           "reading it the declared way stays quiet");

        Serial.clear();
        bool asBool = p.value;
        ok(asBool && !Serial.saw("carries text"),
           "a bool never warns: not-empty is a meaning that holds for every tag");
    }
    {
        uint8_t frame[64];
        uint8_t payload[4];
        float v = 23.4f;
        memcpy(payload, &v, 4);
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_FLOAT, payload, 4);
        Parsed p = parse(frame, n);

        Serial.clear();
        float asFloat = p.value; (void)asFloat;
        ok(!Serial.saw("carries text"), "a float read as a float says nothing");

        Serial.clear();
        const char* asText = p.value; (void)asText;
        ok(Serial.saw("not a text signal"), "…and the mirror case warns too");
    }

    section("Les charges utiles qui ne tiennent pas leur promesse");
    {
        uint8_t frame[64];
        uint8_t half[2] = {0x33, 0x33};
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, SIGNAL_TAG_FLOAT, half, 2);
        Parsed p = parse(frame, n);
        ok(!p.recognised, "half a float is not a smaller float — it is refused");
    }
    {
        uint8_t frame[64];
        uint8_t b[1] = {1};
        size_t n = codec.encodeSignal(frame, sizeof(frame), 1, 0x7F, b, 1);
        Parsed p = parse(frame, n);
        ok(!p.recognised, "a tag we do not know is refused, not guessed");
    }

    section("Le routage");
    {
        i5Calls = i5CallsSecond = i0Calls = weakCalls = 0;

        ok(route(GOLD_FLOAT_23_4_AT_I5, sizeof(GOLD_FLOAT_23_4_AT_I5)), "a signal frame is claimed");
        ok(i5Calls == 1, "the block on I5 ran");
        okNear(i5Last, 23.4f, "…with the value");
        ok(i5CallsSecond == 1, "a second block on the same address runs too");
        ok(i0Calls == 0, "the block on I0 did not — one byte apart is a different signal");
        ok(weakCalls == 1, "onSignalWritten saw it once");

        route(GOLD_BOOL_TRUE_AT_I0, sizeof(GOLD_BOOL_TRUE_AT_I0));
        ok(i0Calls == 1 && i0Last, "…and I0 runs when I0 is written");
        ok(i5Calls == 1, "I5 was not called again");
    }
    {
        uint8_t frame[64];
        uint8_t payload[4] = {0, 0, 0, 0};
        size_t n = codec.encodeSignal(frame, sizeof(frame), 99, SIGNAL_TAG_FLOAT, payload, 4);
        int before = weakCalls;
        ok(route(frame, n), "an address nobody listens to is still a valid frame");
        ok(weakCalls == before + 1, "…and reaches the catch-all, which is how you notice");
    }
    {
        uint8_t frame[64];
        const char* mode = "ECO";
        size_t n = codec.encodeSignal(frame, sizeof(frame), 9, SIGNAL_TAG_STRING,
                                      (const uint8_t*)mode, strlen(mode));
        route(frame, n);
        ok(strcmp(i9Last, "ECO") == 0, "a text signal reaches a const char* capture");
    }
    {
        // The regression the new branch could have caused: swallowing frames
        // that were never signals in the first place.
        btnCalls = 0;
        uint8_t frame[128];
        size_t n = codec.encode(frame, sizeof(frame), "dev1", "btn1",
                                TYPE_SIMPLEBUTTON, CMD_PRESS);
        core.processFrame(frame, n);
        ok(btnCalls == 1, "a button press still reaches its block, through the same core");
    }

    printf("\n%d checks, %d failure%s\n", checks, failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
