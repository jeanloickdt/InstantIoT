#pragma once
/**
 * ============================================================
 *  BinaryCodec.hpp — iWidgets v1 binary codec
 * ============================================================
 *
 * Protocol:
 *   AA | VER | LEN(2B LE) | DEV_COUNT | [DEV_LEN|DEV]×N
 *      | WID_LEN | WID | TYPE | EVENT | PAYLOAD | CRC8
 *
 * No sequence number — all supported transports
 * (TCP/WebSocket/BT RFCOMM/BT BLE/Serial) guarantee reliable
 * ordering without duplication. An application-level SEQ
 * would bring no robustness and caused drops after
 * app or device restart (desynchronized counters).
 *
 * EVENT code convention:
 *   0x01..0x0E = Device → App (push events)
 *   0x10..0x1F = App → Device (received commands)
 *
 * CRC-8/SMBUS poly=0x07
 * Strings: uint8 LEN + bytes
 * Floats : IEEE 754 little-endian
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <string.h>
#include "../InstantIoTConfig.h"

namespace iiot {

// ============================================================
//  TYPE CODES
// ============================================================

static const uint8_t TYPE_SIMPLEBUTTON     = 0x01;
static const uint8_t TYPE_ADVANCEDBUTTON   = 0x02;
static const uint8_t TYPE_GAUGE            = 0x03;
static const uint8_t TYPE_JOYSTICK         = 0x04;
static const uint8_t TYPE_HLEVEL           = 0x05;
static const uint8_t TYPE_VLEVEL           = 0x06;
static const uint8_t TYPE_METRIC           = 0x07;
static const uint8_t TYPE_SEGSWITCH        = 0x08;
static const uint8_t TYPE_ADVANCEDCHART    = 0x09;
static const uint8_t TYPE_HSLIDER          = 0x0A;
static const uint8_t TYPE_VSLIDER          = 0x0B;
static const uint8_t TYPE_LED              = 0x0C;
static const uint8_t TYPE_SWITCH           = 0x0D;
static const uint8_t TYPE_DIRECTIONPAD     = 0x0E;
static const uint8_t TYPE_TEXT             = 0x0F;
static const uint8_t TYPE_BARCHART          = 0x10;
static const uint8_t TYPE_EMERGENCYBUTTON   = 0x11;

// Service frame: periodic heartbeat emitted by the device in
// TCP Server mode. The server receives → does not dispatch to apps,
// simply resets its soTimeout. Format: WID_LEN=0, TYPE=0xFE, EVENT=0,
// empty payload.
static const uint8_t TYPE_HEARTBEAT         = 0xFE;

// InstantIoT 2.0 — the value path. Free: widget types stop at 0x11.
static const uint8_t TYPE_SIGNAL            = 0x20;

// An EVENT — a fact, not a state. Same layout as a signal: a one-byte
// address in the WID slot, and the EVENT slot finally carrying an event
// (CMD_PRESS, CMD_TOGGLE…), which is what it was made for.
//
// Why it cannot be a signal: a value is idempotent. Three presses write
// 1, 1, 1 — only one transition is observable, so two
// appuis disparaissent. Aucun réglage ne comble cet écart.
static const uint8_t SIGNAL_TAG_BOOL        = 0x01;
static const uint8_t SIGNAL_TAG_INT         = 0x02;
static const uint8_t SIGNAL_TAG_FLOAT       = 0x03;
static const uint8_t SIGNAL_TAG_STRING      = 0x04;

/**
 * The tag's high bit: this value is a RESTORE, not a live write.
 *
 * The board reconnects and the server sends back its last value. The frame
 * is otherwise identical to what a finger just wrote — without this mark,
 * an `ISimpleButton(I5)` would fire at boot, with nobody having pressed.
 *
 * A state is restored, a gesture is not replayed. That whole difference
 * is what this bit carries.
 */
static const uint8_t SIGNAL_TAG_RESTORE     = 0x80;

// ============================================================
//  EVENT CODES — Device → App (0x01..0x0E)
// ============================================================

static const uint8_t EV_SETVALUE           = 0x01;
static const uint8_t EV_SETRANGE           = 0x02;
static const uint8_t EV_UPDATE             = 0x03;
static const uint8_t EV_SETBRIGHTNESS      = 0x04;
static const uint8_t EV_SETCOLOR           = 0x05;
static const uint8_t EV_SETSECONDARY       = 0x02;
static const uint8_t EV_ADDPOINT           = 0x01;
static const uint8_t EV_ADDTIMEDPOINT      = 0x02;
static const uint8_t EV_CLEARSERIES        = 0x03;
static const uint8_t EV_CLEARALL           = 0x04;
static const uint8_t EV_SETSERIESDATA      = 0x05;
static const uint8_t EV_SETTEXT            = 0x01;

// BarChart (TYPE_BARCHART)
static const uint8_t EV_BAR_SETVALUES      = 0x01;  // [count:u8][values:float×count]
static const uint8_t EV_BAR_SETBAR         = 0x02;  // [index:u8][value:float]
static const uint8_t EV_BAR_CLEAR          = 0x03;  // no payload

// ============================================================
//  COMMAND CODES — App → Device (0x10..0x1F)
// ============================================================

static const uint8_t CMD_PRESS             = 0x01;
static const uint8_t CMD_RELEASE           = 0x02;
static const uint8_t CMD_LONGPRESS         = 0x03;
static const uint8_t CMD_TOGGLE            = 0x04;
static const uint8_t CMD_POSCHANGED        = 0x01;
static const uint8_t CMD_RELEASED          = 0x02;
static const uint8_t CMD_TURNON            = 0x01;
static const uint8_t CMD_TURNOFF           = 0x02;
static const uint8_t CMD_SWITCHVALUE       = 0x04;
static const uint8_t CMD_BTNPRESSED        = 0x01;
static const uint8_t CMD_BTNLONGPRESSED    = 0x02;
static const uint8_t CMD_BTNRELEASED       = 0x03;
static const uint8_t CMD_SELCHANGED        = 0x01;
static const uint8_t CMD_SEGSELECTED       = 0x02;
static const uint8_t CMD_SEGDESELECTED     = 0x03;
static const uint8_t CMD_VALUECHANGING     = 0x10;
static const uint8_t CMD_VALUECHANGED      = 0x11;
static const uint8_t CMD_DRAGSTARTED       = 0x12;
static const uint8_t CMD_DRAGENDED         = 0x13;

// EmergencyButton (TYPE_EMERGENCYBUTTON) — App → Device
static const uint8_t CMD_EMERGENCY_TRIGGER = 0x01;  // no payload
static const uint8_t CMD_EMERGENCY_RESET   = 0x02;  // no payload

// ============================================================
//  CRC-8/SMBUS poly=0x07
// ============================================================

static uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
    }
    return crc;
}

// ============================================================
//  PRIMITIVES — little-endian
// ============================================================

static void writeU16LE(uint8_t* buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

static uint16_t readU16LE(const uint8_t* buf) {
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static void writeFloatLE(uint8_t* buf, float val) {
    uint32_t bits; memcpy(&bits, &val, 4);
    buf[0] = bits & 0xFF;
    buf[1] = (bits >> 8)  & 0xFF;
    buf[2] = (bits >> 16) & 0xFF;
    buf[3] = (bits >> 24) & 0xFF;
}

static float readFloatLE(const uint8_t* buf) {
    uint32_t bits = (uint32_t)buf[0]
                  | ((uint32_t)buf[1] << 8)
                  | ((uint32_t)buf[2] << 16)
                  | ((uint32_t)buf[3] << 24);
    float val; memcpy(&val, &bits, 4);
    return val;
}

/**
 * A length-prefixed string into `room` bytes. Returns 0 — and writes
 * nothing — when it does not fit: a name longer than the wire's one-byte
 * length, or longer than what is left of the body. It used to truncate
 * the length to a byte and copy the whole string anyway, past the end of
 * a stack buffer.
 */
static size_t writeString(uint8_t* buf, size_t room, const char* str) {
    size_t len = str ? strlen(str) : 0;
    if (len > 255 || 1 + len > room) return 0;
    buf[0] = (uint8_t)len;
    if (len) memcpy(buf + 1, str, len);
    return 1 + len;
}

// ============================================================
//  BINARYCODEC
// ============================================================

class BinaryCodec {

    char _deviceId[32];

public:

    BinaryCodec() {
        _deviceId[0] = '\0';
    }

    // ============================================================
    //  ENCODE — payload bytes → complete binary frame
    // ============================================================

    size_t encode(
        uint8_t* buffer,
        size_t bufferSize,
        const char* deviceId,
        const char* widgetId,
        uint8_t typeCode,
        uint8_t eventCode,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) {
        uint8_t body[256];
        size_t b = 0;

        // DEV_COUNT + DEV
        if (deviceId && deviceId[0] != '\0') {
            body[b++] = 1;
            size_t w = writeString(body + b, sizeof(body) - b, deviceId);
            if (w == 0) return 0;
            b += w;
        } else {
            body[b++] = 0;
        }

        // WID_LEN + WID
        size_t w = writeString(body + b, sizeof(body) - b, widgetId);
        if (w == 0) return 0;
        b += w;
        if (b + 2 > sizeof(body)) return 0;

        // TYPE + EVENT
        body[b++] = typeCode;
        body[b++] = eventCode;

        // PAYLOAD
        if (payloadBytes && payloadLen > 0) {
            if (b + payloadLen > sizeof(body)) return 0;
            memcpy(body + b, payloadBytes, payloadLen);
            b += payloadLen;
        }

        uint16_t len = (uint16_t)b;
        uint8_t  crc = crc8(body, b);

        // header(4) + body + crc(1)
        size_t frameSize = 4 + b + 1;
        if (frameSize > bufferSize) return 0;

        size_t pos = 0;
        buffer[pos++] = 0xAA;
        buffer[pos++] = 0x01;
        writeU16LE(buffer + pos, len); pos += 2;
        memcpy(buffer + pos, body, b); pos += b;
        buffer[pos++] = crc;
        return pos;
    }

    // ============================================================
    //  ENCODE — SIGNAL (InstantIoT 2.0)
    // ============================================================

    /**
     * A SIGNAL frame: `InstantIoT.write(I0, 23.4)` on the wire.
     *
     * It rides the layout that already exists — a new TYPE code, exactly how
     * the heartbeat has always cohabited — so gesture frames are untouched and
     * the server's parser needs no fork.
     *
     *   AA | VER | LEN | DEV_COUNT=0 | WID_LEN=1 | addr | TYPE | TAG | value | CRC
     *
     * Two slots are reused rather than added: the address takes the WID field
     * on ONE byte, and the type tag takes the EVENT field. Zero extra byte, and
     * the whole frame is 14 bytes for a float against 19 for a named widget.
     *
     * The board never writes its own id here — DEV_COUNT is 0. The connection
     * is already authenticated by the device token, so repeating the identity
     * on every frame would buy nothing and cost bytes.
     */
    size_t encodeSignal(
        uint8_t* buffer,
        size_t bufferSize,
        uint8_t address,
        uint8_t typeTag,
        const uint8_t* payloadBytes = nullptr,
        size_t payloadLen = 0
    ) {
        uint8_t body[64];
        size_t b = 0;

        body[b++] = 0;          // DEV_COUNT — the relay knows the board
        body[b++] = 1;          // WID_LEN — the address is one byte
        body[b++] = address;
        body[b++] = TYPE_SIGNAL;
        body[b++] = typeTag;

        if (payloadBytes && payloadLen > 0) {
            if (b + payloadLen > sizeof(body)) return 0;
            memcpy(body + b, payloadBytes, payloadLen);
            b += payloadLen;
        }

        uint8_t crc = crc8(body, b);
        size_t frameSize = 4 + b + 1;
        if (frameSize > bufferSize) return 0;

        size_t pos = 0;
        buffer[pos++] = 0xAA;
        buffer[pos++] = 0x01;
        writeU16LE(buffer + pos, (uint16_t)b); pos += 2;
        memcpy(buffer + pos, body, b); pos += b;
        buffer[pos++] = crc;
        return pos;
    }

    // ============================================================
    //  DECODE — binary frame → DecodedMessage
    // ============================================================

    /**
     * A SIGNAL frame, read without the string machinery.
     *
     * `decode()` cannot be used for this: it reads the WID slot as a length-
     * prefixed string, and a signal puts a raw address byte there. `I0` would
     * be read as a zero-length string and `I4` would swallow the four bytes
     * behind it. So the address is read here, as the byte it is.
     *
     * Returns false for anything that is not a well-formed signal frame —
     * including every ordinary widget frame, which is what makes this usable
     * as the discriminator before the general decoder.
     */
    static bool decodeSignal(
        const uint8_t* buffer,
        size_t length,
        uint8_t& outAddress,
        uint8_t& outTag,
        const uint8_t*& outPayload,
        size_t& outPayloadLen,
        /**
         * True if the frame is a restore.
         *
         * Kept separate from `outTag`, which keeps its original meaning —
         * the value's type. Callers that only decode do not change a line;
         * only the one that must decide asks the question.
         */
        bool& outRestore
    ) {
        if (!buffer || length < 6) return false;
        if (buffer[0] != 0xAA || buffer[1] != 0x01) return false;

        uint16_t len = readU16LE(buffer + 2);
        if (length < (size_t)(4 + len + 1)) return false;

        // DEV_COUNT + WID_LEN + address + TYPE + TAG
        if (len < 5) return false;

        const uint8_t* body = buffer + 4;

        // Checked before the CRC so an ordinary widget frame is not charged
        // for a checksum its own decoder is about to compute again.
        if (body[0] != 0x00) return false;   // a signal carries no device list
        if (body[1] != 0x01) return false;   // the address is exactly one byte
        if (body[3] != TYPE_SIGNAL) return false;

        if (crc8(body, len) != body[len]) {
            IIOT_LOG("[Signal] CRC mismatch");
            return false;
        }

        outAddress    = body[2];
        outRestore    = (body[4] & SIGNAL_TAG_RESTORE) != 0;
        outTag        = body[4] & ~SIGNAL_TAG_RESTORE;
        outPayload    = body + 5;
        outPayloadLen = (size_t)len - 5;
        return true;
    }
};

} // namespace iiot