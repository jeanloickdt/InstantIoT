#pragma once
/**
 * ============================================================
 * 🔐 TlsClient_ESP8266.hpp — TLS on eighty kilobytes of RAM
 * ============================================================
 *
 * The ESP8266 has no crypto co-processor and no second core. TLS here is
 * BearSSL, in software, out of the same heap the sketch lives in — which is
 * why this file was the last one written and the only one whose viability
 * had to be MEASURED before it was promised.
 *
 * It fits. The numbers are below, and they were taken, not assumed.
 *
 * ## What a session costs, in bytes
 *
 * Measured by compiling `sizeof` against the ESP8266 core 3.1.2, and read
 * out of the core's own sources:
 *
 *     receive buffer     6144 + 325 overhead   = 6469
 *     transmit buffer     512 +  85 overhead   =  597
 *     br_ssl_client_context                      3064
 *     br_x509_minimal_context                    3408
 *     BearSSL stack thunk (StackThunk.cpp:45)    6200
 *     trust anchors (X1 + X2, parsed)           ~1000
 *                                              ───────
 *                                              ~20 700
 *
 * A NodeMCU with WiFi associated and a sketch of this size has roughly
 * 40 KB of free heap, so a session takes about half of it and gives most of
 * it back when it closes. That is comfortable, not generous: a sketch that
 * also holds a 10 KB JSON document will run out, and it will look like a
 * failed handshake.
 *
 * The core's own default is `setBufferSizes(16384, 512)` — 17 KB for the
 * receive buffer alone, 30 KB in total. That is the number that gave the
 * ESP8266 its reputation for not being able to do TLS.
 *
 * ## Why 6144 and not 512, and why it is not a free choice
 *
 * A TLS record has to fit in the receive buffer WHOLE. The way to make a
 * small buffer legal is RFC 6066 *max_fragment_length*: the client asks for
 * 512, 1024, 2048 or 4096, and the server agrees to never exceed it.
 * BearSSL asks for it on its own as soon as the buffer is under 16 KB.
 *
 * **instantiot.cloud does not answer.** Probed on 2 September 2026:
 *
 *     openssl s_client -connect instantiot.cloud:9443 -tls1_2 \
 *             -maxfraglen 512 -trace
 *
 * The ServerHello came back with session_ticket, renegotiate,
 * extended_master_secret, ec_point_formats and server_name — and no
 * max_fragment_length. The TLS gateway is Go's `crypto/tls`, which does not
 * implement the extension at all. Asking harder will not change that.
 *
 * So the buffer has to hold whatever the server actually sends, and the
 * same probe says what that is: the Certificate handshake record was
 * **3411 bytes in one piece**. 4096 would work today with 685 bytes to
 * spare — and Let's Encrypt has just lengthened that chain to four
 * certificates. 6144 leaves room for about three more before a handshake
 * starts failing on a day nobody deployed anything.
 *
 * After the handshake the records are our frames, and a frame is at most
 * `INSTANT_MAX_FRAME_SIZE` bytes. The gateway is a byte pipe: it writes
 * what the relay writes, so the records stay small. That is a property of
 * the deployment, not of TLS — if a server on the other end ever buffers
 * and sends 16 KB at once, this buffer is the thing that breaks.
 *
 * ## A certificate has dates, so the board needs a clock
 *
 * BearSSL refuses a certificate it cannot date. The core only passes a time
 * down when one was set (`WiFiClientSecureBearSSL.cpp:1098`), and without
 * it BearSSL falls back to `time(NULL)` — which on a board that just booted
 * is 1 January 1970, forty years before the certificate's notBefore. The
 * handshake then fails for a reason that has nothing to do with the
 * network, and says so in a way nobody reads as "set the clock".
 *
 * So this transport gets the time before it opens a session, and it does
 * NOT invent one: it asks an NTP server, once, when the WiFi comes up, and
 * waits — through the ordinary backoff, not a `delay()` — until the answer
 * is plausible. A sketch that already has a clock (an RTC, its own
 * `configTime`) is left alone: the check is on the clock, not on who set
 * it.
 *
 * `INSTANTIOT_SNTP_SERVER` names the server, and `pool.ntp.org` is the
 * default because it has to be something.
 *
 * ## What this file does NOT contain
 *
 * Session, handshake, retry, backoff, the in-flight rule and the four
 * `ITransport` methods: all in [TcpSession]. What is here is TLS and the
 * clock — everything the other transports do not have to think about.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP8266) && !defined(ESP8266)
#  error "TlsClient_ESP8266.hpp requires ESP8266"
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>
#include <memory>
#include "../TcpSession.hpp"
#include "../../certs/InstantIoT_LE_Roots.h"

/**
 * The receive buffer, in bytes — the one number that matters here.
 *
 * See the header: it must hold the largest TLS record the server sends in
 * one piece, and instantiot.cloud does not negotiate a smaller maximum.
 * 3411 bytes measured, 6144 chosen. Raising it costs heap byte for byte;
 * lowering it below ~4 KB breaks the handshake, not the throughput.
 */
#ifndef INSTANTIOT_TLS_RX_BUFFER
  #define INSTANTIOT_TLS_RX_BUFFER 6144
#endif

/**
 * The transmit buffer. 512 is the core's own floor, and we send a handshake
 * of at most 256 bytes followed by frames of `INSTANT_MAX_FRAME_SIZE`.
 */
#ifndef INSTANTIOT_TLS_TX_BUFFER
  #define INSTANTIOT_TLS_TX_BUFFER 512
#endif

/** Where the board asks what time it is. See the header. */
#ifndef INSTANTIOT_SNTP_SERVER
  #define INSTANTIOT_SNTP_SERVER "pool.ntp.org"
#endif

/**
 * The floor below which the clock is "not set yet": 1 January 2025.
 *
 * A board that has never heard from an NTP server reports 1970. Any real
 * answer is years past this. It is a plausibility test, not a security
 * one — BearSSL does the real date checking against the certificate.
 */
#ifndef INSTANTIOT_CLOCK_PLAUSIBLE_AFTER
  #define INSTANTIOT_CLOCK_PLAUSIBLE_AFTER 1735689600UL
#endif

namespace iiot {

class TlsClient_ESP8266 : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    TlsClient_ESP8266(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : TcpSession(sslClient_, serverIp, serverPort, token)
      , ssid_(nullptr)
      , pass_(nullptr)
      , caPem_(nullptr)          // null = the embedded Let's Encrypt roots
      , insecure_(false)
    {}

    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ----- TLS trust — call BEFORE begin() -----

    /**
     * Your own root, for a server with its own authority.
     *
     * The identity is still verified — against what you supply instead of
     * against Let's Encrypt. The anchors are parsed on the first connection
     * attempt, not here: a PEM costs heap, and a sketch that never connects
     * should not pay for it.
     */
    void setCACert(const char* pem) {
        caPem_    = pem;
        insecure_ = false;
        anchors_.reset();   // rebuilt on the next attempt
    }

    /**
     * Encrypt without checking who is at the other end.
     *
     * MITM is then possible: this is for a first bring-up, not for what
     * stays plugged in. It also removes the need for a clock — nothing
     * dated is being checked any more.
     */
    void setInsecure() {
        insecure_ = true;
        IIOT_LOG("[TLS8266] ⚠️ INSECURE — the server's identity is NOT verified");
    }

    IPAddress getLocalIP()      const { return WiFi.localIP(); }
    bool      isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

protected:

    bool linkUp() const override { return WiFi.status() == WL_CONNECTED; }

    /** The ONLY place that calls `WiFi.begin` — [TcpSession] guards it. */
    bool beginLink() override {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        return true;   // in flight: the association negotiates in background
    }

    /** Without `disconnect()` the next `begin()` hits the same error. */
    void endLinkAttempt() override { WiFi.disconnect(); }

    /**
     * The WiFi is up — which is the first moment NTP can be asked.
     *
     * Called on every pass, so it must be cheap and it must not restart
     * anything: the guard is `sntpStarted_`, and `configTime` is a
     * one-shot that leaves the SDK polling on its own afterwards.
     */
    void onLinkUp() override {
        if (insecure_ || sntpStarted_ || clockIsSet()) return;
        IIOT_LOG("[TLS8266] Asking " INSTANTIOT_SNTP_SERVER " what time it is");
        configTime(0, 0, INSTANTIOT_SNTP_SERVER);
        sntpStarted_ = true;
    }

    /**
     * Not before the clock is set — see the header.
     *
     * Returning false sends [TcpSession] through its normal backoff, so the
     * board waits a second, looks again, and connects as soon as NTP has
     * answered. No `delay()`, and no handshake spent on a certificate we
     * could not have dated.
     */
    bool readyToConnect() const override {
        if (insecure_ || clockIsSet()) return true;
        IIOT_LOG("[TLS8266] Waiting for the clock before the TLS handshake");
        return false;
    }

    /**
     * Everything BearSSL needs, set on EVERY attempt.
     *
     * `setInsecure()` and `setTrustAnchors()` each clear the other's
     * settings in the core, and a reconnection builds a fresh engine — so
     * these are not one-time calls, and treating them as such is how a
     * transport reconnects into an unverified session.
     */
    void prepareClient() override {
        sslClient_.setBufferSizes(INSTANTIOT_TLS_RX_BUFFER,
                                  INSTANTIOT_TLS_TX_BUFFER);
        sslClient_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);

        if (insecure_) {
            sslClient_.setInsecure();
            return;
        }

        if (!anchors_) {
            anchors_.reset(new BearSSL::X509List(
                caPem_ ? caPem_ : INSTANTIOT_LE_ROOT_CAS));
        }
        sslClient_.setTrustAnchors(anchors_.get());
        sslClient_.setX509Time(time(nullptr));
    }

    bool linkCredentialsReady() const override {
        if (ssid_ && pass_) return true;
        IIOT_LOG("[TLS8266] Missing WiFi credentials");
        return false;
    }

private:

    /** Has anyone — NTP, an RTC, the sketch — told this board the date? */
    static bool clockIsSet() {
        return (uint32_t)time(nullptr) > INSTANTIOT_CLOCK_PLAUSIBLE_AFTER;
    }

    const char* ssid_;
    const char* pass_;
    const char* caPem_;
    bool        insecure_;
    bool        sntpStarted_ = false;

    /** Built on the first attempt, kept for the life of the transport. */
    std::unique_ptr<BearSSL::X509List> anchors_;

    BearSSL::WiFiClientSecure sslClient_;
};

} // namespace iiot
