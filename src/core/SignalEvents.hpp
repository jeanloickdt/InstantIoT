#pragma once
/*
 * InstantIoT — receiving a signal.
 *
 * The mirror of `InstantIoT.write(I0, v)`: the app, a rule, or the server
 * replaying a setpoint after a reboot writes a value, and the board reacts.
 *
 *     ISignal(I5) {
 *         WHEN_WRITTEN(float target) { setpoint = target; }
 *     };
 *
 * Two things are deliberately different from the widget registry next door.
 *
 * The key is the **address**, one byte, compared with `==`. A widget handler
 * costs a `strcmp` per registered block per frame; a signal costs a byte
 * compare. That is the whole reason the address exists.
 *
 * And the handler is not typed by the signal. The board cannot know what the
 * server declared, so the value carries every reading of itself and the
 * capture picks one: `WHEN_WRITTEN(bool on)` reads it as a bool,
 * `WHEN_WRITTEN(float t)` as a float. Asking for text on a numeric signal is
 * not silent — it says so in the debug log.
 *
 * Copyright (c) 2026 InstantIoT — MIT License
 */

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "InstantIoTSignals.hpp"
#include "BinaryCodec.hpp"

namespace InstantIoT {

// ============================================================
//  A VALUE THAT DOES NOT KNOW WHAT IT WILL BE READ AS
// ============================================================

/**
 * Integral targets read the integer, everything else reads the float.
 *
 * Without this, a 32-bit counter arriving as an int and captured as a `long`
 * would round-trip through a float and lose its low bits past 2^24 — the kind
 * of loss nobody notices until the totals stop adding up.
 */
template <typename T> struct SignalIsIntegral { enum { value = 0 }; };
#define _IIO_SIGNAL_INTEGRAL(T) \
    template <> struct SignalIsIntegral<T> { enum { value = 1 }; };
_IIO_SIGNAL_INTEGRAL(char)
_IIO_SIGNAL_INTEGRAL(signed char)
_IIO_SIGNAL_INTEGRAL(unsigned char)
_IIO_SIGNAL_INTEGRAL(short)
_IIO_SIGNAL_INTEGRAL(unsigned short)
_IIO_SIGNAL_INTEGRAL(int)
_IIO_SIGNAL_INTEGRAL(unsigned int)
_IIO_SIGNAL_INTEGRAL(long)
_IIO_SIGNAL_INTEGRAL(unsigned long)
_IIO_SIGNAL_INTEGRAL(long long)
_IIO_SIGNAL_INTEGRAL(unsigned long long)
#undef _IIO_SIGNAL_INTEGRAL
// `bool` is absent on purpose: it has its own conversion below, so that 0.5
// reads as true rather than truncating to 0 first.

template <bool Integral> struct SignalCastTo;
template <> struct SignalCastTo<true> {
    template <typename T> static T from(long i, float)   { return static_cast<T>(i); }
};
template <> struct SignalCastTo<false> {
    template <typename T> static T from(long, float f)   { return static_cast<T>(f); }
};

/**
 * The value as it arrived, readable as whatever the sketch asks for.
 *
 * The conversion operator is a template so that *any* target is an exact
 * match — `uint8_t brightness = e.value` would be ambiguous between a handful
 * of fixed operators, and the error message for that is unreadable.
 */
struct SignalValue {
    uint8_t     tag     = SIGNAL_TAG_FLOAT;
    long        integer = 0;
    float       number  = 0.0f;
    bool        flag    = false;
    const char* string  = "";

    template <typename T>
    operator T() const {
        // A text signal read as a number would hand back 0 without a word.
        // Nothing else can catch this: the server owns the declaration, the
        // sketch owns the capture, and no compiler sees both.
        if (tag == SIGNAL_TAG_STRING)
            IIOT_LOG("[Signal] read as a number, but this signal carries text");
        return SignalCastTo<SignalIsIntegral<T>::value != 0>::template from<T>(integer, number);
    }

    /**
     * True when the signal is not zero and not empty — a meaning that holds
     * for all four tags, which is why this one never warns.
     */
    operator bool()        const { return flag; }
    operator const char*() const { return text(); }
    operator String()      const { return String(text()); }

    /** The text, and a word in the log if this signal never carried any. */
    const char* text() const {
        if (tag != SIGNAL_TAG_STRING)
            IIOT_LOG("[Signal] read as text, but this signal is not a text signal");
        return string;
    }
};

/** What a sketch receives when someone writes to one of its addresses. */
struct SignalEvent {
    uint8_t     address = 0;
    SignalValue value;
};

/**
 * Reads the payload of a SIGNAL frame into [out].
 *
 * [textBuf] is where a text payload is copied and terminated — the frame
 * carries a length, not a NUL, and the caller owns the buffer so nothing here
 * touches the heap. Returns false on a tag we do not know or a payload too
 * short for its tag, which is the only honest answer: a truncated float is not
 * a smaller float.
 */
inline bool decodeSignalValue(
    uint8_t tag,
    const uint8_t* payload,
    size_t payloadLen,
    SignalValue& out,
    char* textBuf,
    size_t textBufSize
) {
    out.tag = tag;

    switch (tag) {
        case SIGNAL_TAG_BOOL: {
            if (payloadLen < 1) return false;
            out.flag    = payload[0] != 0;
            out.integer = out.flag ? 1 : 0;
            out.number  = out.flag ? 1.0f : 0.0f;
            return true;
        }
        case SIGNAL_TAG_INT: {
            if (payloadLen < 4) return false;
            uint32_t bits = (uint32_t)payload[0]
                          | ((uint32_t)payload[1] << 8)
                          | ((uint32_t)payload[2] << 16)
                          | ((uint32_t)payload[3] << 24);
            int32_t v   = (int32_t)bits;
            out.integer = (long)v;
            out.number  = (float)v;
            out.flag    = v != 0;
            return true;
        }
        case SIGNAL_TAG_FLOAT: {
            if (payloadLen < 4) return false;
            uint32_t bits = (uint32_t)payload[0]
                          | ((uint32_t)payload[1] << 8)
                          | ((uint32_t)payload[2] << 16)
                          | ((uint32_t)payload[3] << 24);
            float f;
            memcpy(&f, &bits, sizeof(f));
            out.number  = f;
            out.integer = (long)f;
            out.flag    = f != 0.0f;
            return true;
        }
        case SIGNAL_TAG_STRING: {
            if (!textBuf || textBufSize == 0) return false;
            size_t n = payloadLen < (textBufSize - 1) ? payloadLen : (textBufSize - 1);
            memcpy(textBuf, payload, n);
            textBuf[n]  = '\0';
            out.string  = textBuf;
            out.flag    = n > 0;
            return true;
        }
    }

    IIOT_LOG("[Signal] unknown type tag — frame ignored");
    return false;
}

// ============================================================
//  THE REGISTRY — one byte compare, no strings anywhere
// ============================================================

struct SignalHandler {
    uint8_t address;
    void (*fn)(const SignalEvent&);
    SignalHandler* next;
};

inline SignalHandler*& signalHandlerListHead() {
    static SignalHandler* head = nullptr;
    return head;
}

/** Attaches one `ISignal(...)` block to the list, before `setup()` runs. */
struct SignalRegistrar {
    SignalHandler node;
    SignalRegistrar(SignalRef ref, void (*fn)(const SignalEvent&)) {
        node.address = ref.addr;
        node.fn      = fn;
        node.next    = signalHandlerListHead();
        signalHandlerListHead() = &node;
    }
};

/** @return combien de blocs ont été appelés — zéro se diagnostique. */
inline uint8_t dispatchSignal(const SignalEvent& e) {
    uint8_t called = 0;
    for (SignalHandler* h = signalHandlerListHead(); h; h = h->next) {
        if (h->address == e.address) { h->fn(e); called++; }
    }
    return called;
}

/** Un `ISignal` écoute-t-il cette adresse ? */
inline bool hasSignalHandlerAt(uint8_t address) {
    for (SignalHandler* h = signalHandlerListHead(); h; h = h->next) {
        if (h->address == address) return true;
    }
    return false;
}

}  // namespace InstantIoT

using SignalEvent = InstantIoT::SignalEvent;
using SignalValue = InstantIoT::SignalValue;

/** Called for every signal written to this board, before the `ISignal` blocks. */
__attribute__((weak)) void onSignalWritten(const SignalEvent& e);
