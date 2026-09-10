#pragma once
/*
 * The smallest Arduino.h that lets the codec compile on a laptop.
 *
 * This exists so the frame format can be pinned by a test that runs in a
 * second, instead of by flashing a board. It is deliberately tiny: anything
 * that needs more of Arduino than this belongs in a sketch, not in a unit
 * test.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

typedef uint8_t byte;

/** Just enough String for the conversion operators to be exercised. */
class String {
public:
    String() {}
    String(const char* s) : _s(s ? s : "") {}
    String(int v)   { char b[24]; snprintf(b, sizeof(b), "%d", v);   _s = b; }
    String(long v)  { char b[24]; snprintf(b, sizeof(b), "%ld", v);  _s = b; }
    String(float v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); _s = b; }
    const char* c_str() const { return _s.c_str(); }
    bool operator==(const char* o) const { return _s == (o ? o : ""); }
private:
    std::string _s;
};

#define F(x) (x)
#define PI 3.1415926535897932384626433832795

/** The clock the tests move by hand. One instance for every translation unit. */
inline unsigned long& fakeMillis() { static unsigned long now = 0; return now; }
inline unsigned long millis() { return fakeMillis(); }

/** AVR-isms the codec uses; the C library has no equivalent by that name. */
inline char* dtostrf(double v, signed char width, unsigned char prec, char* out) {
    snprintf(out, 32, "%*.*f", (int)width, (int)prec, v);
    return out;
}
inline char* itoa(int v, char* out, int base) {
    snprintf(out, 12, base == 10 ? "%d" : "%x", v);
    return out;
}

/**
 * A Serial that RECORDS instead of printing.
 *
 * The tests compile with INSTANTIOT_DEBUG on, so the library's diagnostics
 * become assertable text rather than noise on stdout. A warning nobody can
 * test is a warning that quietly stops being emitted.
 */
struct _FakeSerial {
    std::string log;
    void clear() { log.clear(); }
    bool saw(const char* needle) const { return log.find(needle) != std::string::npos; }

    void print(const char* s)   { if (s) log += s; }
    void println(const char* s) { if (s) log += s; log += "\n"; }
    template <typename T> void print(T)   {}
    template <typename T> void println(T) { log += "\n"; }
};
static _FakeSerial Serial;
