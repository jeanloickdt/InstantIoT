#pragma once
/**
 * ============================================================
 * 🎯 Destinations.hpp — who the board talks to
 * ============================================================
 *
 * A destination is a **description**, not a live object: a host, a port,
 * a token, and how to check we are talking to who we think. It is read
 * inside `begin()` and does not outlive it — the facade draws the
 * transport from it and keeps that.
 *
 *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
 *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
 *
 * ## Why two types and not a flag
 *
 * `Cloud(TOKEN)` encrypts; `Cloud(TOKEN).plaintext()` does not. The
 * difference is not a runtime boolean: it changes the type, therefore the
 * transport, therefore what the linker embeds. A board with no TLS stack
 * — they exist, and their owner knows it — can then reach the cloud for
 * real, instead of failing to compile over a path it never takes.
 *
 * Plaintext means plaintext: the token and the values travel readable.
 * That is a decision taken knowingly, never a default.
 * ============================================================
 */

#include <stdint.h>

/** The InstantIoT cloud host. Redefinable for a staging deployment. */
#ifndef INSTANTIOT_CLOUD_HOST
  #define INSTANTIOT_CLOUD_HOST "instantiot.cloud"
#endif

/** TLS gateway. */
#ifndef INSTANTIOT_CLOUD_TLS_PORT
  #define INSTANTIOT_CLOUD_TLS_PORT 9443
#endif

/** Plaintext gateway — the same one a self-hosted server uses. */
#ifndef INSTANTIOT_CLOUD_PLAIN_PORT
  #define INSTANTIOT_CLOUD_PLAIN_PORT 9001
#endif

/** Default heartbeat: the server sets its absence timeout from it. */
#ifndef INSTANTIOT_DEFAULT_HEARTBEAT_MS
  #define INSTANTIOT_DEFAULT_HEARTBEAT_MS 5000
#endif

namespace iiot {

struct SecureDestination;

/**
 * A plaintext destination — bare TCP.
 *
 * This is the normal case for a server you host on your own network,
 * where encryption would protect a journey that never leaves the house.
 * It is also the only possible case for a board with no TLS stack.
 */
struct PlainDestination {
    const char* host;
    uint16_t    port;
    const char* token;
    uint32_t    heartbeatMs = INSTANTIOT_DEFAULT_HEARTBEAT_MS;

    PlainDestination(const char* h, uint16_t p, const char* t)
        : host(h), port(p), token(t) {}

    /** Presence heartbeat interval, in milliseconds. 0 disables it. */
    PlainDestination& heartbeatEvery(uint32_t ms) { heartbeatMs = ms; return *this; }

    /** Switch to encrypted, same host and same port. */
    SecureDestination secure() const;
};

/**
 * An encrypted destination — TLS.
 *
 * The server's identity is verified by default, against the embedded
 * Let's Encrypt roots. Two ways out, and they do not mean the same thing:
 *
 *   `.withCertificate(pem)` — "here is my own root". The identity is
 *   still verified, against an authority you supply. That is what a
 *   self-hosted server with its own certificate needs.
 *
 *   `.withoutCertCheck()` — "I check nothing". The journey stays
 *   encrypted, but anyone can pose as the server. For a first bring-up,
 *   not for what stays plugged in.
 */
struct SecureDestination {
    const char* host;
    uint16_t    port;
    const char* token;
    /** Null = the embedded roots. */
    const char* caPem = nullptr;
    bool        checksIdentity = true;
    uint32_t    heartbeatMs = INSTANTIOT_DEFAULT_HEARTBEAT_MS;
    /** True as soon as the sketch has named a port: `plaintext()` respects it. */
    bool        portWasChosen = false;

    SecureDestination(const char* h, uint16_t p, const char* t)
        : host(h), port(p), token(t) {}

    SecureDestination& withCertificate(const char* pem) { caPem = pem; return *this; }
    SecureDestination& withoutCertCheck() { checksIdentity = false; return *this; }
    SecureDestination& heartbeatEvery(uint32_t ms) { heartbeatMs = ms; return *this; }

    /** A staging deployment, a self-hosted cloud — nothing else moves. */
    SecureDestination& at(const char* h, uint16_t p) {
        host = h; port = p; portWasChosen = true; return *this;
    }

    /**
     * Switch to plaintext.
     *
     * The port follows: if the sketch named none, the TLS gateway's port
     * no longer means anything and gives way to the plaintext one. If the
     * sketch did name one, it is respected — it is the side that knew.
     */
    PlainDestination plaintext() const {
        PlainDestination d(host, portWasChosen ? port : INSTANTIOT_CLOUD_PLAIN_PORT, token);
        d.heartbeatMs = heartbeatMs;
        return d;
    }
};

inline SecureDestination PlainDestination::secure() const {
    SecureDestination d(host, port, token);
    d.heartbeatMs = heartbeatMs;
    d.portWasChosen = true;
    return d;
}

// ════════════════════════════════════════════════════════════
//  The two names a sketch writes
// ════════════════════════════════════════════════════════════

/** The InstantIoT cloud. Encrypted, identity verified. */
inline SecureDestination Cloud(const char* token) {
    return SecureDestination(INSTANTIOT_CLOUD_HOST, INSTANTIOT_CLOUD_TLS_PORT, token);
}

/**
 * A server you host. Plaintext, port 9001 by default.
 *
 * `MyServer` and not `Server`: the Arduino core already has a
 * `class Server`, and the compiler refuses the bare name — the same
 * collision as `WiFi`. The possessive also states the real difference
 * with `Cloud`: hosted by us, or hosted by you.
 */
inline PlainDestination MyServer(const char* host, const char* token) {
    return PlainDestination(host, INSTANTIOT_CLOUD_PLAIN_PORT, token);
}

inline PlainDestination MyServer(const char* host, uint16_t port, const char* token) {
    return PlainDestination(host, port, token);
}

}  // namespace iiot
