#pragma once
/**
 * ============================================================
 * 🔗 EthClient_W5x00.hpp — a cable instead of a radio
 * ============================================================
 *
 * One file, three families. Arduino's `Ethernet` library exposes the same
 * `EthernetClient` on AVR, SAMD and ESP32, and [TcpSession] only ever holds
 * an Arduino `Client` — so the session, the handshake, the retry and the
 * backoff arrive here already written. What is left is the two questions any
 * link has to answer.
 *
 * Covers the W5100, W5200 and W5500 shields and modules. The ENC28J60 is a
 * different chip with a different library, and it is not this file.
 *
 * ## A link has no credentials
 *
 * There is no SSID and no password: a cable is either plugged in or it is
 * not. `linkCredentialsReady()` therefore keeps its default — yes — and the
 * MAC address is the only thing the caller supplies.
 *
 * ## Why it never negotiates in the background
 *
 * `Ethernet.begin(mac)` is a DHCP exchange, and it BLOCKS until it succeeds
 * or times out. There is no in-flight state to watch, which is exactly why
 * [TcpSession::endLinkAttempt] stays empty here: nothing to cut.
 *
 * That also means a first `begin()` on an unplugged board costs the DHCP
 * timeout. The default is one second, deliberately short — the retry loop is
 * the right place to wait, not `setup()`.
 *
 * ## No TLS, and it is not an oversight
 *
 * An AVR has neither the RAM nor the flash for a TLS handshake, and the
 * W5x00 has no crypto of its own. A Mega reaches the cloud in plaintext or
 * not at all; `Links.hpp` says so at compile time rather than letting a
 * sketch discover it on the bench.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#include <Arduino.h>
#include <Ethernet.h>
#include "../TcpSession.hpp"

/**
 * How long `Ethernet.begin()` waits for a DHCP lease, in milliseconds.
 *
 * One second. The board is not idle while it waits — it is BLOCKED — and a
 * cable that is not plugged in will not become plugged in during those
 * milliseconds. The backoff of [TcpSession] is where waiting belongs.
 */
#ifndef INSTANTIOT_ETH_DHCP_TIMEOUT_MS
  #define INSTANTIOT_ETH_DHCP_TIMEOUT_MS 1000
#endif

namespace iiot {

class EthClient_W5x00 : public TcpSession {
public:

    // See the note in TcpClient_ESP32: the base only stores the reference.
    EthClient_W5x00(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : TcpSession(ethClient_, serverIp, serverPort, token)
    {}

    /**
     * The MAC address of the shield.
     *
     * Older shields carry theirs on a sticker; the cheap modules carry none
     * at all, and any locally-administered address will do. The default here
     * is one of those — fine on a home network, and a collision would only
     * happen with a second board using this very library and this very
     * default. Set your own if you run more than one.
     */
    void setMac(const uint8_t mac[6]) {
        for (uint8_t i = 0; i < 6; i++) mac_[i] = mac[i];
    }

    IPAddress getLocalIP() const { return Ethernet.localIP(); }

protected:

    /**
     * The cable AND the lease.
     *
     * `Ethernet.linkStatus()` answers about the cable — and only the W5200
     * and W5500 can answer at all; a W5100 always says `Unknown`. So the
     * test is the IP: with a lease we are on the network, whatever the chip
     * is willing to say about its own socket.
     */
    bool linkUp() const override {
        return Ethernet.localIP() != IPAddress(0, 0, 0, 0);
    }

    /**
     * DHCP, and it blocks.
     *
     * Unlike a WiFi association there is nothing left in flight afterwards:
     * either we have a lease when this returns, or we have nothing. Saying
     * so — returning false — keeps [TcpSession] from starting its in-flight
     * watch on an attempt that is already over.
     */
    bool beginLink() override {
        IIOT_LOG("[Ethernet] DHCP…");
        if (Ethernet.begin(mac_, INSTANTIOT_ETH_DHCP_TIMEOUT_MS) == 0) {
            if (Ethernet.hardwareStatus() == EthernetNoHardware) {
                IIOT_LOG("[Ethernet] No W5x00 found — check the shield is seated");
            } else if (Ethernet.linkStatus() == LinkOFF) {
                IIOT_LOG("[Ethernet] Cable unplugged");
            } else {
                IIOT_LOG("[Ethernet] No DHCP lease");
            }
            return false;
        }
        IIOT_LOG("[Ethernet] Lease obtained");
        // False, even on success: see [TcpSession::beginLink]. It answers
        // "is something still in flight?", not "did it work?" — and DHCP
        // blocked, so nothing is.
        return false;
    }

    /**
     * The lease has to be renewed, and nobody else will do it.
     *
     * `Ethernet.maintain()` is cheap when there is nothing to do — it
     * compares two timestamps — and losing a lease silently would look
     * exactly like a broken server.
     */
    void onLinkUp() override { Ethernet.maintain(); }

private:
    /** Locally-administered by default — see [setMac]. */
    uint8_t     mac_[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    EthernetClient ethClient_;
};

} // namespace iiot
