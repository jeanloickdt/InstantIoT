#pragma once
/**
 * ============================================================
 *  BinaryCodec.hpp — Codec binaire iWidgets v1
 * ============================================================
 *
 * Protocole :
 *   AA | VER | LEN(2B LE) | DEV_COUNT | [DEV_LEN|DEV]×N
 *      | WID_LEN | WID | TYPE | EVENT | PAYLOAD | CRC8
 *
 * Convention codes EVENT :
 *   0x01..0x0E = Device → App (events push)
 *   0x0F       = patch  (envoi groupé multi-champs)
 *   0x10..0x1F = App → Device (commands reçues)
 *
 * Optimisation mémoire — activer seulement les widgets utilisés :
 *
 *   #define INSTANTIOT_WIDGET_SIMPLEBUTTON
 *   #define INSTANTIOT_WIDGET_ADVANCEDBUTTON
 *   #define INSTANTIOT_WIDGET_GAUGE
 *   #define INSTANTIOT_WIDGET_JOYSTICK
 *   #define INSTANTIOT_WIDGET_HLEVEL
 *   #define INSTANTIOT_WIDGET_VLEVEL
 *   #define INSTANTIOT_WIDGET_METRIC
 *   #define INSTANTIOT_WIDGET_SEGSWITCH
 *   #define INSTANTIOT_WIDGET_ADVANCEDCHART
 *   #define INSTANTIOT_WIDGET_HSLIDER
 *   #define INSTANTIOT_WIDGET_VSLIDER
 *   #define INSTANTIOT_WIDGET_LED
 *   #define INSTANTIOT_WIDGET_SWITCH
 *   #define INSTANTIOT_WIDGET_DIRECTIONPAD
 *   #define INSTANTIOT_WIDGET_TEXT
 *
 * Si aucun #define n'est présent, tous les widgets sont compilés
 * (comportement par défaut, rétrocompatible).
 *
 * CRC-8/SMBUS poly=0x07
 * Strings : uint8 LEN + octets
 * Floats  : IEEE 754 little-endian
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <string.h>
#include "Codec.h"
#include "../InstantIoTConfig.h"

// ── Activation par défaut si aucun widget explicite ───────────────────────────
#if !defined(INSTANTIOT_WIDGET_SIMPLEBUTTON)   && \
    !defined(INSTANTIOT_WIDGET_ADVANCEDBUTTON) && \
    !defined(INSTANTIOT_WIDGET_GAUGE)          && \
    !defined(INSTANTIOT_WIDGET_JOYSTICK)       && \
    !defined(INSTANTIOT_WIDGET_HLEVEL)         && \
    !defined(INSTANTIOT_WIDGET_VLEVEL)         && \
    !defined(INSTANTIOT_WIDGET_METRIC)         && \
    !defined(INSTANTIOT_WIDGET_SEGSWITCH)      && \
    !defined(INSTANTIOT_WIDGET_ADVANCEDCHART)  && \
    !defined(INSTANTIOT_WIDGET_HSLIDER)        && \
    !defined(INSTANTIOT_WIDGET_VSLIDER)        && \
    !defined(INSTANTIOT_WIDGET_LED)            && \
    !defined(INSTANTIOT_WIDGET_SWITCH)         && \
    !defined(INSTANTIOT_WIDGET_DIRECTIONPAD)   && \
    !defined(INSTANTIOT_WIDGET_TEXT)
    #define INSTANTIOT_ALL_WIDGETS
#endif

namespace InstantIoT {

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

// ============================================================
//  EVENT CODES — Device → App (0x01..0x0E) + patch (0x0F)
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
static const uint8_t EV_PATCH              = 0x0F;

// ============================================================
//  COMMAND CODES — App → Device (0x10..0x1F)
// ============================================================

static const uint8_t CMD_PRESS             = 0x01;
static const uint8_t CMD_RELEASE           = 0x02;
static const uint8_t CMD_LONGPRESS         = 0x03;
static const uint8_t CMD_TOGGLE            = 0x04;
static const uint8_t CMD_SETTOGGLESTATE    = 0x10;
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

// ============================================================
//  PATCH FIELD CODES
// ============================================================

static const uint8_t PATCH_VALUE           = 0x01;
static const uint8_t PATCH_MIN_MAX         = 0x02;
static const uint8_t PATCH_UNIT            = 0x03;
static const uint8_t PATCH_SECONDARY       = 0x04;
static const uint8_t PATCH_COLOR           = 0x05;
static const uint8_t PATCH_BRIGHTNESS      = 0x06;
static const uint8_t PATCH_TEXT            = 0x07;

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
//  PRIMITIFS — little-endian
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

static size_t writeString(uint8_t* buf, const char* str) {
    if (!str) { buf[0] = 0; return 1; }
    uint8_t len = (uint8_t)strlen(str);
    buf[0] = len;
    memcpy(buf + 1, str, len);
    return 1 + len;
}

static size_t readString(const uint8_t* buf, char* out, size_t outSize) {
    uint8_t len = buf[0];
    size_t copy = (len < outSize - 1) ? len : outSize - 1;
    memcpy(out, buf + 1, copy);
    out[copy] = '\0';
    return 1 + len;
}

// ============================================================
//  BINARYCODEC
// ============================================================

class BinaryCodec {

    char _deviceId[32];
    char _widgetId[INSTANTIOT_MAX_WIDGET_ID_LENGTH];
    char _paramKeys[8][16];
    char _paramValues[8][32];

    void addParam(DecodedMessage& msg, const char* key, const char* value) {
        if (msg.paramCount >= 8) return;
        uint8_t i = msg.paramCount;
        strncpy(_paramKeys[i],   key,   15); _paramKeys[i][15]   = '\0';
        strncpy(_paramValues[i], value, 31); _paramValues[i][31] = '\0';
        msg.params[i].key   = _paramKeys[i];
        msg.params[i].value = _paramValues[i];
        msg.paramCount++;
    }

    void addParamFloat(DecodedMessage& msg, const char* key, float val) {
        char buf[16]; dtostrf(val, 1, 6, buf);
        addParam(msg, key, buf);
    }

    void addParamInt(DecodedMessage& msg, const char* key, int val) {
        char buf[12]; itoa(val, buf, 10);
        addParam(msg, key, buf);
    }

    void addParamBool(DecodedMessage& msg, const char* key, bool val) {
        addParam(msg, key, val ? "true" : "false");
    }

    void decodePayload(
        uint8_t typeCode, uint8_t eventCode,
        const uint8_t* payload, size_t len,
        DecodedMessage& msg
    ) {
        size_t p = 0;

        // ── PATCH 0x0F — champs multiples ────────────────────────────
        if (eventCode == EV_PATCH) {
            if (p >= len) return;
            uint8_t fieldCount = payload[p++];
            for (uint8_t f = 0; f < fieldCount && p < len; f++) {
                uint8_t fc = payload[p++];
                switch (fc) {
                    case PATCH_VALUE:
                        if (p + 4 <= len) { addParamFloat(msg, "value", readFloatLE(payload+p)); p += 4; }
                        break;
                    case PATCH_MIN_MAX:
                        if (p + 8 <= len) {
                            addParamFloat(msg, "min", readFloatLE(payload+p)); p += 4;
                            addParamFloat(msg, "max", readFloatLE(payload+p)); p += 4;
                        }
                        break;
                    case PATCH_UNIT: {
                        char unit[32];
                        p += readString(payload+p, unit, sizeof(unit));
                        addParam(msg, "unit", unit);
                        break;
                    }
                    case PATCH_SECONDARY: {
                        char val[32], lbl[32];
                        p += readString(payload+p, val, sizeof(val));
                        p += readString(payload+p, lbl, sizeof(lbl));
                        addParam(msg, "value", val);
                        addParam(msg, "label", lbl);
                        break;
                    }
                    case PATCH_COLOR:
                        if (p + 3 <= len) {
                            addParamInt(msg, "r", payload[p++]);
                            addParamInt(msg, "g", payload[p++]);
                            addParamInt(msg, "b", payload[p++]);
                        }
                        break;
                    case PATCH_BRIGHTNESS:
                        if (p < len) addParamInt(msg, "brightness", payload[p++]);
                        break;
                    case PATCH_TEXT: {
                        char txt[64];
                        p += readString(payload+p, txt, sizeof(txt));
                        addParam(msg, "text", txt);
                        break;
                    }
                }
            }
            return;
        }

        // ── Events fixes par TYPE+EVENT ───────────────────────────────
        switch (typeCode) {

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_SIMPLEBUTTON) || defined(INSTANTIOT_WIDGET_ADVANCEDBUTTON)
            case TYPE_SIMPLEBUTTON:
            case TYPE_ADVANCEDBUTTON:
                if ((eventCode == CMD_TOGGLE || eventCode == CMD_SETTOGGLESTATE) && p < len)
                    addParamBool(msg, "state", payload[p++] != 0);
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_GAUGE) || defined(INSTANTIOT_WIDGET_HLEVEL) || defined(INSTANTIOT_WIDGET_VLEVEL)
            case TYPE_GAUGE:
            case TYPE_HLEVEL:
            case TYPE_VLEVEL:
                if (eventCode == EV_SETVALUE && p+4<=len)
                    { addParamFloat(msg,"value",readFloatLE(payload+p)); p+=4; }
                else if (eventCode == EV_SETRANGE && p+8<=len)
                    { addParamFloat(msg,"min",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"max",readFloatLE(payload+p)); p+=4; }
                else if (eventCode == EV_UPDATE && p+12<=len)
                    { addParamFloat(msg,"value",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"min",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"max",readFloatLE(payload+p)); p+=4; }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_JOYSTICK)
            case TYPE_JOYSTICK:
                if (eventCode == CMD_POSCHANGED && p+8<=len)
                    { addParamFloat(msg,"x",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"y",readFloatLE(payload+p)); p+=4; }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_METRIC)
            case TYPE_METRIC:
                if (eventCode == EV_SETVALUE && p+4<=len)
                    { addParamFloat(msg,"value",readFloatLE(payload+p)); p+=4; }
                else if (eventCode == EV_SETSECONDARY && p<len) {
                    char val[32], lbl[32];
                    p += readString(payload+p, val, sizeof(val));
                    p += readString(payload+p, lbl, sizeof(lbl));
                    addParam(msg,"value",val); addParam(msg,"label",lbl);
                }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_SEGSWITCH)
            case TYPE_SEGSWITCH:
                if (eventCode == CMD_SELCHANGED && p<len) {
                    addParamInt(msg,"index",payload[p++]);
                    char ids[64]; p += readString(payload+p, ids, sizeof(ids));
                    addParam(msg,"ids",ids);
                } else if ((eventCode==CMD_SEGSELECTED||eventCode==CMD_SEGDESELECTED) && p<len)
                    addParamInt(msg,"index",payload[p++]);
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_ADVANCEDCHART)
            case TYPE_ADVANCEDCHART:
                if (eventCode == EV_ADDPOINT && p<len) {
                    char sid[32]; p += readString(payload+p,sid,sizeof(sid)); addParam(msg,"seriesId",sid);
                    if (p+4<=len) { addParamFloat(msg,"y",readFloatLE(payload+p)); p+=4; }
                } else if (eventCode == EV_ADDTIMEDPOINT && p<len) {
                    char sid[32]; p += readString(payload+p,sid,sizeof(sid)); addParam(msg,"seriesId",sid);
                    if (p+8<=len) { addParamFloat(msg,"x",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"y",readFloatLE(payload+p)); p+=4; }
                } else if (eventCode == EV_CLEARSERIES && p<len) {
                    char sid[32]; p += readString(payload+p,sid,sizeof(sid)); addParam(msg,"seriesId",sid);
                }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_HSLIDER) || defined(INSTANTIOT_WIDGET_VSLIDER)
            case TYPE_HSLIDER:
            case TYPE_VSLIDER:
                if (eventCode == EV_SETRANGE && p+8<=len)
                    { addParamFloat(msg,"min",readFloatLE(payload+p)); p+=4; addParamFloat(msg,"max",readFloatLE(payload+p)); p+=4; }
                else if (p+4<=len)
                    { addParamFloat(msg,"value",readFloatLE(payload+p)); p+=4; }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_LED)
            case TYPE_LED:
                if (eventCode == EV_SETBRIGHTNESS && p<len)
                    addParamInt(msg,"brightness",payload[p++]);
                else if (eventCode == EV_SETCOLOR && p+3<=len)
                    { addParamInt(msg,"r",payload[p++]); addParamInt(msg,"g",payload[p++]); addParamInt(msg,"b",payload[p++]); }
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_SWITCH)
            case TYPE_SWITCH:
                if (eventCode == CMD_SWITCHVALUE && p<len)
                    addParamBool(msg,"isOn",payload[p++]!=0);
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_DIRECTIONPAD)
            case TYPE_DIRECTIONPAD:
                if (p<len) addParamInt(msg,"button",payload[p++]);
                break;
#endif

#if defined(INSTANTIOT_ALL_WIDGETS) || defined(INSTANTIOT_WIDGET_TEXT)
            case TYPE_TEXT:
                if (eventCode == EV_SETTEXT && p<len) {
                    char txt[64]; p += readString(payload+p,txt,sizeof(txt));
                    addParam(msg,"text",txt);
                }
                break;
#endif

        }
    }

public:

    BinaryCodec() {
        _deviceId[0] = '\0';
        _widgetId[0] = '\0';
    }

    // ============================================================
    //  ENCODE — bytes payload → trame binaire complète
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
            b += writeString(body + b, deviceId);
        } else {
            body[b++] = 0;
        }

        // WID_LEN + WID
        b += writeString(body + b, widgetId);

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
    //  DECODE — trame binaire → DecodedMessage
    // ============================================================

    bool decode(
        const uint8_t* buffer,
        size_t length,
        DecodedMessage& outMessage,
        uint8_t& outTypeCode,
        uint8_t& outEventCode
    ) {
        if (!buffer || length < 9) return false;
        size_t pos = 0;

        if (buffer[pos++] != 0xAA) return false;
        if (buffer[pos++] != 0x01) return false;

        uint16_t len = readU16LE(buffer + pos); pos += 2;
        if (length < (size_t)(4 + len + 1)) return false;

        // CRC
        if (crc8(buffer + 4, len) != buffer[4 + len]) {
            IIOT_LOG("[BinaryCodec] CRC mismatch");
            return false;
        }

        size_t bodyStart = pos;

        // DEV_COUNT + devices
        uint8_t devCount = buffer[pos++];
        _deviceId[0] = '\0';
        for (uint8_t d = 0; d < devCount; d++) {
            if (d == 0) pos += readString(buffer + pos, _deviceId, sizeof(_deviceId));
            else { uint8_t dlen = buffer[pos++]; pos += dlen; }
        }

        // WID
        pos += readString(buffer + pos, _widgetId, sizeof(_widgetId));

        // TYPE + EVENT
        outTypeCode  = buffer[pos++];
        outEventCode = buffer[pos++];

        // PAYLOAD
        size_t payloadLen = (bodyStart + len) - pos;
        outMessage.paramCount = 0;
        if (payloadLen > 0)
            decodePayload(outTypeCode, outEventCode, buffer + pos, payloadLen, outMessage);

        outMessage.deviceId    = _deviceId;
        outMessage.widgetId    = _widgetId;
        outMessage.widgetType  = "";
        outMessage.event       = "";
        outMessage.dashboardId = "";

        return true;
    }
};

} // namespace InstantIoT