#pragma once
/**
 * ============================================================
 * 🔄 InstantIoTCore.hpp - Main loop — binary protocol v1
 * ============================================================
 */

#include <Arduino.h>
#include "Transport.h"
#include "BinaryCodec.hpp"
#include "InstantIoTSignals.hpp"
#include "SignalEvents.hpp"
#include "InstantIoTDeviceConfig.hpp"
#include "InstantIoTMessage.hpp"
#include "MessageSender.h"
#include "../InstantIoTConfig.h"

namespace iiot {

class InstantIoTCoreBase : public IMessageSender {
public:

    InstantIoTCoreBase(ITransport& transport)
        : _transport(transport)
        , _rxPos(0)
        , _begun(false)
    {}

    /**
     * Nothing left to free.
     *
     * It used to release eight heap-allocated widget arrays — one per
     * display family, filled by the `gauge("name")` factories. Displays now
     * read signals: the board writes an address, and nothing on the sketch
     * side represents the drawing.
     */
    virtual ~InstantIoTCoreBase() = default;

    // ════════════════════════════════════════════════════════
    //  LIFECYCLE
    // ════════════════════════════════════════════════════════

    /**
     * `_begun` says `begin()` was ATTEMPTED, not that the link is open.
     *
     * The distinction cost a mute board in the field. It used to mean "the
     * transport answered yes": a WiFi not yet up, a server not answering,
     * and it stayed false — so `loop()` returned at once, so `poll()` was
     * never called. And `poll()` is what carries the reconnection with
     * backoff. The board never tried again, until the reset button, while
     * the sketch calmly printed "not connected" forever.
     *
     * Failing on the first attempt is NORMAL: at boot the router is not
     * always ready, and the server not always reachable. Giving up is what
     * must not be.
     */
    virtual bool begin() {
        IIOT_LOG("[InstantIoT] Starting...");
        _begun = true;
        if (!_transport.begin()) {
            IIOT_LOG("[InstantIoT] Transport FAILED — loop() will keep trying");
            return false;
        }
        IIOT_LOG("[InstantIoT] Ready");
        return true;
    }

    virtual void loop() {
        if (!_begun) return;
        _transport.poll();
        readLoop();
        heartbeatTick();
    }

    // ════════════════════════════════════════════════════════
    // 💓 HEARTBEAT
    // ════════════════════════════════════════════════════════
    //
    // Configures the periodic emission of `TYPE_HEARTBEAT` (0xFE) frames
    // to signal the device's presence to the server in TCP mode.
    // The server in parallel sets its `soTimeout = heartbeatMs × 2.5`:
    // if no frame (widget OR heartbeat) is received within this window,
    // it declares the device offline and broadcasts to apps.
    //
    // `intervalMs = 0` disables emission (legacy, useful for
    // BLE / WiFi-AP modes where heartbeat has no meaning).
    //
    // The `intervalMs` parameter must **match** the one passed to the
    // transport at handshake — the `InstantIoTWiFiServer` facade
    // handles it automatically.
    void setHeartbeat(uint32_t intervalMs) {
        _heartbeatMs = intervalMs;
        _lastHeartbeatSent = 0;  // force a quick emission after set
    }

    bool connected() override {
        return _transport.connected();
    }

    /** How many frames arrived that the board could not read. */
    uint32_t ignoredFrames() const { return _ignoredFrames; }

    /**
     * One place decides whether a signal frame leaves.
     *
     * A single global ceiling, not a per-signal budget: the constraint comes
     * from the platform, never from the user, and one counter is all a small
     * board should spend on it.
     */
    bool sendSignal(uint8_t address, uint8_t tag, const uint8_t* payload, size_t len) {
        if (!_transport.connected()) return false;

        uint32_t now = millis();
        uint32_t minGap = 1000UL / (_signalRatePerSecond ? _signalRatePerSecond : 1);
        if (_lastSignalAt != 0 && (now - _lastSignalAt) < minGap) return false;

        size_t n = _codec.encodeSignal(_txBuffer, sizeof(_txBuffer), address, tag, payload, len);
        if (n == 0) return false;
        if (_transport.write(_txBuffer, n) != n) return false;

        _lastSignalAt = now;
        return true;
    }

    bool sendBinary(
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) override {
        if (!_transport.connected()) return false;

        size_t len = _codec.encode(
            _txBuffer, sizeof(_txBuffer),
            _config.getDeviceId(),
            widgetId,
            typeCode,
            eventCode,
            payloadBytes,
            payloadLen
        );

        if (len == 0) return false;
        return _transport.write(_txBuffer, len) == len;
    }

    // ════════════════════════════════════════════════════════
    // 📡 SIGNALS (InstantIoT 2.0) — the value path
    // ════════════════════════════════════════════════════════

    /**
     * Sends a measure: `InstantIoT.write(I0, readTemp())`.
     *
     * The sketch may call this on every pass of `loop()`. The library applies
     * the platform ceiling — `caps["messages.perSecond"]`, the very number the
     * server's fuse enforces — so a beginner writing in the main loop can no
     * longer be disconnected for flooding. It is COOPERATIVE: the server never
     * trusts it, the fuse stays.
     *
     * @return true when a frame actually left. `false` means the ceiling
     *         swallowed this call, which is not an error.
     */
    bool write(SignalRef sig, float value) {
        uint8_t p[4];
        writeFloatLE(p, value);
        return sendSignal(sig.addr, SIGNAL_TAG_FLOAT, p, 4);
    }

    bool write(SignalRef sig, bool value) {
        uint8_t p[1] = { (uint8_t)(value ? 1 : 0) };
        return sendSignal(sig.addr, SIGNAL_TAG_BOOL, p, 1);
    }

    bool write(SignalRef sig, int value) {
        uint8_t p[4];
        p[0] = (uint8_t)(value & 0xFF);
        p[1] = (uint8_t)((value >> 8) & 0xFF);
        p[2] = (uint8_t)((value >> 16) & 0xFF);
        p[3] = (uint8_t)((value >> 24) & 0xFF);
        return sendSignal(sig.addr, SIGNAL_TAG_INT, p, 4);
    }

    /**
     * The convenience overloads — and they are not a convenience.
     *
     * Without them, `write(I0, analogRead(A0) * 3.3 / 4095.0)` does NOT
     * compile: the expression is a `double`, and `float`, `bool` and `int`
     * become three candidates tied. The compiler talks about an ambiguous
     * overload, which helps nobody understand that writing `3.3f` would
     * have been enough.
     *
     * But writing `3.3` rather than `3.3f` is what everybody does, and
     * `millis()` returns an `unsigned long`. These lines exist so that the
     * most natural thing to write is the thing that compiles.
     */
    bool write(SignalRef sig, double value)        { return write(sig, (float)value); }
    bool write(SignalRef sig, long value)          { return write(sig, (int)value); }
    bool write(SignalRef sig, unsigned long value) { return write(sig, (int)value); }
    bool write(SignalRef sig, unsigned int value)  { return write(sig, (int)value); }
    bool write(SignalRef sig, short value)         { return write(sig, (int)value); }
    bool write(SignalRef sig, unsigned short value){ return write(sig, (int)value); }
    bool write(SignalRef sig, unsigned char value) { return write(sig, (int)value); }

    bool write(SignalRef sig, const char* value) {
        if (!value) return false;
        size_t n = strlen(value);
        if (n > 48) n = 48;   // a signal carries a value, not a document
        return sendSignal(sig.addr, SIGNAL_TAG_STRING, (const uint8_t*)value, n);
    }

    /**
     * The platform ceiling, in frames per second.
     *
     * Compiled default until the server pushes the real one at connection
     * (étape 4) — deliberately the same value the server's fuse uses, so a
     * board that never hears from the server still behaves.
     */
    void setSignalRateLimit(uint16_t framesPerSecond) {
        _signalRatePerSecond = framesPerSecond ? framesPerSecond : 1;
    }

    // ════════════════════════════════════════════════════════
    // ⚙️ CONFIG
    // ════════════════════════════════════════════════════════

    DeviceConfig& config() { return _config; }

protected:
    ITransport&  _transport;
    BinaryCodec  _codec;
    DeviceConfig _config;

    uint8_t _rxBuffer[INSTANT_RX_BUFFER_SIZE];
    size_t  _rxPos;
    uint8_t _txBuffer[INSTANT_TX_BUFFER_SIZE];

    // ── Signals (2.0) ───────────────────────────────────────
    // The platform ceiling and the single counter that applies it. One global
    // pair, not one per address: the constraint is the platform's, and a small
    // board should not pay a table for it.
    uint16_t _signalRatePerSecond = INSTANTIOT_DEFAULT_SIGNAL_RATE;
    uint32_t _lastSignalAt = 0;

    /** True as soon as `begin()` has been called — not as soon as the link holds. */
    bool _begun;

    /** See `processFrame`. */
    uint32_t _ignoredFrames = 0;

    // ─── Heartbeat state (server mode) ────────────────────
    uint32_t _heartbeatMs       = 0;   // 0 = disabled
    uint32_t _lastHeartbeatSent = 0;

    /**
     * To be called in `loop()`: sends a `TYPE_HEARTBEAT` frame
     * if the configured interval has elapsed. No-op if `_heartbeatMs == 0`
     * or if the transport is not connected.
     */
    void heartbeatTick() {
        if (_heartbeatMs == 0) return;
        if (!_transport.connected()) return;
        uint32_t now = millis();
        if (now - _lastHeartbeatSent < _heartbeatMs) return;
        _lastHeartbeatSent = now;
        // Empty WID + TYPE_HEARTBEAT + EVENT 0 + no payload
        sendBinary("", TYPE_HEARTBEAT, 0);
    }

    // ════════════════════════════════════════════════════════
    // 📥 READ — binary frame reassembly
    // ════════════════════════════════════════════════════════

    void readLoop() {
        while (_transport.available() > 0) {
            uint8_t buf[64];
            int n = _transport.read(buf, sizeof(buf));
            if (n <= 0) break;
            for (int i = 0; i < n; i++) {
                if (_rxPos < sizeof(_rxBuffer)) {
                    _rxBuffer[_rxPos++] = buf[i];
                } else {
                    _rxPos = 0;
                    IIOT_LOG("[Core] RX overflow, reset");
                }
            }
        }
        extractFrames();
    }

    void extractFrames() {
        // Header = AA + VER + LEN(2) = 4 bytes. Min frame = header + CRC = 5 bytes.
        while (_rxPos >= 5) {
            if (_rxBuffer[0] != 0xAA) { shiftBuffer(1); continue; }
            if (_rxBuffer[1] != 0x01) { shiftBuffer(1); continue; }

            uint16_t len = (uint16_t)_rxBuffer[2] | ((uint16_t)_rxBuffer[3] << 8);
            // sanity guard: header(4) + body(len) + crc(1) must fit in the buffer
            if (len > sizeof(_rxBuffer) - 5) { shiftBuffer(1); continue; }
            uint16_t frameSize = 4 + len + 1;  // header(4) + body(len) + crc(1)

            if (_rxPos < frameSize) break;

            processFrame(_rxBuffer, frameSize);
            shiftBuffer(frameSize);
        }
    }

    void shiftBuffer(size_t n) {
        if (n >= _rxPos) { _rxPos = 0; return; }
        memmove(_rxBuffer, _rxBuffer + n, _rxPos - n);
        _rxPos -= n;
    }

    /**
     * A frame we cannot handle is INFORMATION.
     *
     * It used to vanish without a word. Two things can explain it, and
     * both deserve to be known: the other end speaks a language we have
     * stopped understanding — that happened with EVENT frames, which the
     * app still sent once the board had stopped reading them — or it is
     * noise on the wire.
     *
     * The counter is there because a log is not read after the fact: a
     * sketch can publish `InstantIoT.ignoredFrames()` on a signal and see
     * the problem from the app.
     */
    void processFrame(const uint8_t* data, size_t len) {
        if (dispatchSignalFrame(data, len)) return;

        if (_ignoredFrames == 0) {
            IIOT_LOG("[InstantIoT] unrecognised frame — the other end speaks "
                     "a language this version does not read");
        }
        _ignoredFrames++;
    }


    /**
     * Returns true once the frame has been recognised as a signal — whether or
     * not a handler was listening, and whether or not the payload made sense.
     * A malformed signal is still a signal; handing it to the widget decoder
     * afterwards could only produce nonsense.
     */
    bool dispatchSignalFrame(const uint8_t* data, size_t len) {
        uint8_t address = 0, tag = 0;
        const uint8_t* payload = nullptr;
        size_t payloadLen = 0;

        bool rappel = false;
        if (!BinaryCodec::decodeSignal(data, len, address, tag, payload, payloadLen, rappel))
            return false;

        SignalEvent e;
        e.address = address;
        if (!decodeSignalValue(tag, payload, payloadLen, e.value,
                               _signalText, sizeof(_signalText)))
            return true;

        onSignalWritten(e);

        // Un RAPPEL ne réveille pas un geste.
        //
        // On reconnect the server sends back the last value of every
        // signal that asks for it. That is what a state needs — a
        // setpoint, a threshold: `ISignal(I5, float t)` must find it.
        //
        // But `ISimpleButton(I5)` declares something else: "I want to know
        // somebody pressed". Nobody pressed. Delivering it would invent a
        // gesture, and the sketch would light a lamp nobody asked for.
        //
        // The rule therefore belongs to the BLOCK, not to the signal's
        // settings: whether replay is ticked or not, a gesture is not
        // replayed.
        dispatchSignal(e, rappel);
        return true;
    }


    static void formatAddress(uint8_t v, char* out, size_t) {
        if (v >= 100) { *out++ = '0' + (v / 100); v %= 100; *out++ = '0' + (v / 10); }
        else if (v >= 10) { *out++ = '0' + (v / 10); }
        *out++ = '0' + (v % 10);
        *out = '\0';
    }

    /** `I255` + NUL. */
    char _eventRef[5] = {0};

    /**
     * Where a text payload is terminated. 48 characters is what the server
     * accepts, so a longer one was already cut before it reached the wire.
     */
    char _signalText[49] = {0};
};

} // namespace iiot