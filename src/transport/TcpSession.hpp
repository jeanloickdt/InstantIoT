#pragma once
/**
 * ============================================================
 * 🔌 TcpSession.hpp — everything a TCP transport does that is not the link
 * ============================================================
 *
 * `TcpClient_ESP32` and `TcpClient_R4` were the same file twice. Session,
 * handshake, retry, backoff with jitter, the in-flight rule, the four
 * `ITransport` reads and writes — identical. What actually differed was two
 * questions: *is the link up?* and *bring it up*.
 *
 * This class is everything else, and the two questions are pure virtual.
 *
 * ## The client is an Arduino `Client`
 *
 * `WiFiClient`, `EthernetClient` and `WiFiSSLClient` all derive from it, so
 * the session never knows which one it holds. That is what lets one trunk
 * serve WiFi, Ethernet and TLS without a branch.
 *
 * ## Four variation points, not two — and why
 *
 * Two are the ones that matter: [linkUp] and [beginLink]. The other two are
 * empty by default, and exist because the ESP32 can say WHY a WiFi
 * association failed while the R4 cannot — its WiFiS3 has no event API. A
 * link that has nothing to say overrides nothing and writes nothing.
 *
 * ## The in-flight rule is trunk, not variation
 *
 * It looks WiFi-specific and it is not: both clients carried it, word for
 * word. Calling `begin()` on an association that is still being negotiated
 * does not restart it — it KILLS it and starts from zero, and a board on a
 * slow network then never arrives. The rule is: watch, do not touch.
 *
 * It cost a real debugging session on real hardware. It lives here now, in
 * one place, and moving it back into a subclass would lose it twice.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <Client.h>
#include "../core/Transport.h"
#include "../InstantIoTConfig.h"

#ifndef INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS 15000
#endif

#ifndef INSTANTIOT_TCP_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_TCP_CONNECT_TIMEOUT_MS 5000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MIN_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MIN_MS 1000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MAX_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MAX_MS 30000
#endif

// ± jitter applied to the backoff to avoid the "thundering herd":
// if N devices in a fleet restart at the same time (power outage, server
// reboot), without jitter they all retry in sync → flood the server. With
// jitter (±25% by default), retries desynchronize naturally.
#ifndef INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT
  #define INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT 25
#endif

// How long a session must HOLD before the backoff forgets its history.
// A connection the server accepts then drops within seconds (bad token,
// fuse, full relay) is not a success: without this window every such drop
// reset the backoff to its minimum, and a rejected board hammered the
// relay once a second forever.
#ifndef INSTANTIOT_SESSION_HELD_MS
  #define INSTANTIOT_SESSION_HELD_MS 10000
#endif

namespace iiot {

class TcpSession : public ITransport {
public:

    TcpSession(Client& client, const char* serverIp, uint16_t serverPort, const char* token)
      : client_(client)
      , serverIp_(serverIp)
      , serverPort_(serverPort)
      , token_(token)
      , nextRetryAt_(0)
      , backoffMs_(INSTANTIOT_RECONNECT_BACKOFF_MIN_MS)
      , heartbeatMs_(0)  // 0 = legacy handshake (no announcement)
    {}

    // ============================================================
    // 💓 Heartbeat — called by the facade before begin()
    // ============================================================
    //
    // Configures the heartbeat interval announced to the server in the
    // handshake. The server adapts its soTimeout (= heartbeat × 2.5).
    // `0` = do not announce (legacy mode, server fallback 90s).
    void setHeartbeat(uint32_t intervalMs) { heartbeatMs_ = intervalMs; }
    uint32_t getHeartbeat() const { return heartbeatMs_; }

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!linkCredentialsReady()) return false;
        if (!waitForLink())          return false;
        seedJitter();
        if (!connectServer())        return false;

        backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
        sessionOpened();
        return true;
    }

    void poll() override {
        // Nothing to retry without credentials — and on most stacks
        // `begin(nullptr)` is unforgiving. The guard used to live in
        // `begin()` alone; now that `loop()` runs `poll()` even after a
        // failed `begin()`, it must be here too.
        if (!linkCredentialsReady()) return;

        if (!linkUp()) {
            if (client_) client_.stop();

            // The chip may know WHY, and know at once: a refused key comes
            // back in two seconds. Saying it here rather than at the
            // timeout is thirteen seconds less spent wondering.
            onLinkDown();

            if (linkAttemptStartedAt_ != 0) {
                // An attempt is IN FLIGHT: watch, do not touch.
                if (millis() - linkAttemptStartedAt_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                // It has lasted long enough. Cut it cleanly — without a
                // proper stop the next attempt hits the same error — and
                // let the backoff decide when to try again.
                IIOT_LOG("[TcpSession] Link attempt timed out — will retry");
                endLinkAttempt();
                linkAttemptStartedAt_ = 0;
                scheduleRetry();
                return;
            }

            if (!retryDue()) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[TcpSession] Link reconnect attempt #", retryAttempt_);
            startLinkAttempt();
            return;   // we will look again on the next pass
        }

        // The link is up: nothing in flight, and the previous reason no
        // longer applies.
        linkAttemptStartedAt_ = 0;
        onLinkUp();

        // TCP dropped → reconnect (with backoff)
        if (!client_.connected()) {
            if (client_) client_.stop();
            if (!retryArmed_) {
                // First pass after the drop: the retry waits its turn like
                // any other, jitter included. Reconnecting at once made a
                // whole fleet knock on the relay in the same second when
                // it came back.
                retryArmed_ = true;
                scheduleRetry();
                return;
            }
            if (!retryDue()) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[TcpSession] TCP reconnect attempt #", retryAttempt_);
            if (!connectServer()) {
                scheduleRetry();
                return;
            }
            sessionOpened();
            return;
        }

        // Connected. The backoff only forgets its history once the session
        // has HELD: an accept followed by a drop is not a success.
        if (!sessionHeld_ && millis() - connectedAt_ >= INSTANTIOT_SESSION_HELD_MS) {
            sessionHeld_ = true;
            backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
            retryAttempt_ = 0;
        }
    }

    // ============================================================
    // 📡 STATUS · 📥 READ · 📤 WRITE
    // ============================================================

    bool connected() override { return linkUp() && client_.connected(); }
    int  available() override { return connected() ? client_.available() : 0; }

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return client_.read(buf, len);
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return client_.write(buf, len);
    }

    const char* getServerIP() const { return serverIp_; }
    uint16_t    getPort()     const { return serverPort_; }

protected:

    // ── The two questions a link must answer ────────────────────

    /** Is the link carrying traffic? `WiFi.status()==WL_CONNECTED`, `ETH.linkUp()`. */
    virtual bool linkUp() const = 0;

    /**
     * Start bringing the link up. `WiFi.begin(ssid,pass)`, `Ethernet.begin(mac)`.
     *
     * @return **true if an attempt is now IN FLIGHT** — asynchronous, to be
     *         watched. **false if it is already over**, successful or not.
     *
     * The two are not the same thing, and getting them backwards costs
     * either a busy loop or a fifteen-second stall. A WiFi association
     * returns true: it negotiates in the background. A DHCP exchange returns
     * false: it blocked, and by the time it returns there is a lease or
     * there is nothing.
     */
    virtual bool beginLink() = 0;

    // ── Two hooks most links leave empty ────────────────────────

    /** The link just dropped. The ESP32 says why, once per distinct reason. */
    virtual void onLinkDown() {}

    /** The link just came up. The ESP32 forgets the reason it last said. */
    virtual void onLinkUp() {}

    /**
     * Cut an attempt that has run out of time.
     *
     * Default: nothing. WiFi stacks need an explicit `disconnect()` — without
     * it the next `begin()` hits the same error — and Ethernet does not.
     */
    virtual void endLinkAttempt() {}

    /**
     * Set the client up just before `connect()`.
     *
     * Default: NOTHING, and that default is the careful one. The ESP32 wants
     * a connect timeout and `setNoDelay(true)`; the Uno R4 must NOT get a
     * timeout at all — a non-zero one switches `WiFiClient::connect()` to a
     * temperamental modem command instead of WiFiS3's reliable path.
     *
     * A trunk that set a timeout for everyone would have broken the R4 in a
     * way no test on this machine can see: it only shows up on the board.
     */
    virtual void prepareClient() {}

    /**
     * Tune the socket once it is OPEN — keepalive, mostly.
     *
     * The board never expects anything from the server, so a server that
     * dies without a FIN — power cut, VM frozen, NAT entry expired — left
     * the board "connected" for minutes while the relay had declared it
     * offline within seconds. TCP keepalive is what notices, and only the
     * stacks with a real socket underneath can turn it on. Default: nothing.
     */
    virtual void tuneSession() {}

    /**
     * Are the credentials there at all?
     *
     * Default: yes. Ethernet needs none; WiFi refuses to start without an
     * SSID, and says so once rather than looping on a null pointer.
     */
    virtual bool linkCredentialsReady() const { return true; }

    /**
     * May the session be opened NOW, the link being up?
     *
     * Default: yes — for every link so far, an address was the only
     * prerequisite. The ESP8266's TLS is the first one with another: BearSSL
     * refuses a certificate it cannot date, and a board that just booted
     * thinks it is 1970.
     *
     * Returning false is not an error: it costs one backoff interval and
     * the attempt comes back. The override is the one that says why — the
     * trunk cannot know, and a "TCP connect FAILED" for a clock that has
     * not arrived yet would send the reader to the router.
     */
    virtual bool readyToConnect() const { return true; }

    /** The ONLY place that starts an attempt, and it records when. */
    void startLinkAttempt() {
        if (beginLink()) {
            linkAttemptStartedAt_ = millis();
        } else {
            // The attempt is already over. Nothing to watch — so the backoff
            // has to be armed HERE, or a link that fails synchronously (a
            // DHCP with no cable) would retry on every single pass, each one
            // blocking for its own timeout.
            scheduleRetry();
        }
    }

    /** The blocking form, for `begin()`. */
    bool waitForLink() {
        IIOT_LOG("[TcpSession] Link connecting");
        startLinkAttempt();

        // A synchronous link has already finished. If it is not up now, it
        // will not become up by being waited on — and fifteen seconds of
        // `delay(100)` in `setup()` for a cable nobody plugged in is fifteen
        // seconds the sketch never gets back.
        if (linkAttemptStartedAt_ == 0) return linkUp();

        uint32_t start = millis();
        while (!linkUp()) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // The attempt stays IN FLIGHT: the stack keeps trying, and
                // `poll()` will let it finish rather than restart it.
                IIOT_LOG("[TcpSession] Link timeout — the attempt continues in the background");
                return false;
            }
            delay(100);
        }

        linkAttemptStartedAt_ = 0;
        onLinkUp();
        IIOT_LOG("[TcpSession] Link OK");
        return true;
    }

    Client&     client_;

private:

    // ----- TCP + handshake -----
    bool connectServer() {
        // Before the log line, so it does not announce a connection that is
        // not being attempted.
        if (!readyToConnect()) return false;

        IIOT_LOG_2("[TcpSession] TCP connecting: ", serverIp_, ":", serverPort_);

        prepareClient();
        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[TcpSession] TCP connect FAILED");
            return false;
        }
        tuneSession();

        // Handshake: [PAYLOAD_LEN | PAYLOAD_BYTES]
        //   payload = "token"            (legacy, heartbeatMs_ = 0)
        //   payload = "token:heartbeat"  (heartbeat enabled)
        if (!token_) {
            IIOT_LOG("[TcpSession] Missing device token");
            client_.stop();
            return false;
        }

        // Build the payload (max 255 bytes length-prefixed)
        char payload[288];
        int written = 0;
        if (heartbeatMs_ > 0) {
            written = snprintf(payload, sizeof(payload), "%s:%lu",
                               token_, (unsigned long)heartbeatMs_);
        } else {
            written = snprintf(payload, sizeof(payload), "%s", token_);
        }
        if (written <= 0 || written > 255) {
            IIOT_LOG("[TcpSession] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[TcpSession] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[TcpSession] Handshake sent, heartbeat=", (long)heartbeatMs_);
        return true;
    }

    // ----- Session bookkeeping -----
    void sessionOpened() {
        connectedAt_  = millis();
        sessionHeld_  = false;
        retryArmed_   = false;
    }

    /** Wrap-safe: `millis()` rolls over after 49 days, a plain `<` does not survive it. */
    bool retryDue() const { return (int32_t)(millis() - nextRetryAt_) >= 0; }


    // ----- Jitter entropy -----
    //
    // On AVR, SAMD and Renesas, `random()` is a plain PRNG that starts from
    // the same state at every boot: a fleet powered up together produced
    // the same "jitter" and knocked on the relay in the same second. The
    // time the link took to come up is the one thing that differs from
    // board to board. ESP32 and ESP8266 draw from hardware, and calling
    // `randomSeed()` there would DOWNGRADE them to the PRNG.
    void seedJitter() {
#if !defined(ESP32) && !defined(ESP8266)
        uint32_t seed = micros() ^ (millis() << 16);
        randomSeed(seed ? seed : 1);
#endif
    }

    // ----- Backoff with jitter -----
    //
    // Computes the next retry = backoffMs_ ± jitter%, then doubles
    // backoffMs_ for next time (cap at MAX). Jitter desynchronizes retries
    // across a device fleet.
    //
    // Logs a warning when the MAX cap is reached — a sign of a persistent
    // issue (server down, broken WiFi config) that the maker should look at.
    void scheduleRetry() {
        uint32_t base = backoffMs_;
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;  // floor: avoid spin

        nextRetryAt_ = millis() + (uint32_t)actualDelay;

        IIOT_LOG_2("[TcpSession] Next retry in ", actualDelay, "ms (base ", base);

        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL(
                    "[TcpSession] Reached max backoff — persistent issue, attempt #",
                    retryAttempt_
                );
            }
            next = INSTANTIOT_RECONNECT_BACKOFF_MAX_MS;
        }
        backoffMs_ = next;
    }

    const char* serverIp_;
    uint16_t    serverPort_;
    const char* token_;

    uint32_t    nextRetryAt_;
    uint32_t    backoffMs_;
    uint32_t    retryAttempt_ = 0;
    /** When the current session opened; meaningful only while connected. */
    uint32_t    connectedAt_ = 0;
    /** True once the session has lasted INSTANTIOT_SESSION_HELD_MS. */
    bool        sessionHeld_ = false;
    /** True once the drop has been noticed and a retry scheduled. */
    bool        retryArmed_ = false;
    /** When the last attempt started, or 0 if nothing is in flight. */
    uint32_t    linkAttemptStartedAt_ = 0;
    uint32_t    heartbeatMs_;       // 0 = legacy, >0 = announced to server
};

} // namespace iiot
