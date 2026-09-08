#pragma once
/**
 * ============================================================
 * 🔌 Links.hpp — how the board reaches the network
 * ============================================================
 *
 * A link is the **path**, a destination is the **far end**. `begin()`
 * reads in the order things happen: the board joins a network first,
 * then reaches a server.
 *
 *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), Cloud(TOKEN));
 *     InstantIoT.begin(WiFiLink("MyWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
 *
 * Three links have no far end to name: the app is already at the other
 * end of the wire. They stand alone, and the first argument does not
 * change.
 *
 *     InstantIoT.begin(AccessPoint("MyBoard", "12345678"));
 *     InstantIoT.begin(BluetoothLink("MyBoard"));
 *     InstantIoT.begin(SerialLink(10, 11));
 *
 * ## The suffix
 *
 * `WiFiLink` and not `WiFi`: the Arduino core already has a global object
 * called `WiFi`, and two things do not share a name. The suffix also says
 * what it is — a path, not a destination — which is exactly the
 * distinction this file exists to hold.
 *
 * ## Ethernet
 *
 * `InstantIoT.begin(EthernetLink(), Cloud(TOKEN))` changes nothing
 * elsewhere: a destination does not know how it is reached. What the cable
 * CAN carry, though, depends on the board — see the note on
 * `EthernetLink::transportTo` below, and the header of `EthLink_ESP32.hpp`.
 *
 * ## What each board can do
 *
 * ESP32        AccessPoint, WiFiLink, EthernetLink (both plain and TLS),
 *              BluetoothLink, BLELink
 * Uno R4 WiFi  AccessPoint, WiFiLink (plain and TLS), EthernetLink (plain)
 * NINA three   AccessPoint, WiFiLink (plain and TLS), EthernetLink (plain)
 * ESP8266      AccessPoint, WiFiLink (plain and TLS), EthernetLink (plain),
 *              SerialLink
 * AVR          EthernetLink (plain), SerialLink
 *
 * A pairing the board cannot do fails to compile, and says so in one
 * sentence rather than a page of templates.
 * ============================================================
 */

#include "Destinations.hpp"
#include "core/Transport.h"

// ════════════════════════════════════════════════════════════
//  What the platform provides
// ════════════════════════════════════════════════════════════

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #include "transport/wifi/SoftAP_ESP32.hpp"
    #include "transport/wifi/TcpClient_ESP32.hpp"
    #include "transport/wifi/TlsClient_ESP32.hpp"
    namespace iiot {
        using TransportAP         = SoftAP_ESP32;
        using TransportWiFiPlain  = TcpClient_ESP32;
        using TransportWiFiSecure = TlsClient_ESP32;
    }
    #define INSTANTIOT_HAS_ACCESS_POINT 1
    #define INSTANTIOT_HAS_WIFI_LINK    1
    #define INSTANTIOT_HAS_TLS          1

#elif defined(ARDUINO_UNOWIFIR4)
    #include "transport/wifi/SoftAP_R4.hpp"
    #include "transport/wifi/TcpClient_R4.hpp"
    #include "transport/wifi/TlsClient_R4.hpp"
    namespace iiot {
        using TransportAP         = SoftAP_R4;
        using TransportWiFiPlain  = TcpClient_R4;
        using TransportWiFiSecure = TlsClient_R4;
    }
    #define INSTANTIOT_HAS_ACCESS_POINT 1
    #define INSTANTIOT_HAS_WIFI_LINK    1
    #define INSTANTIOT_HAS_TLS          1

#elif defined(ARDUINO_SAMD_MKRWIFI1010) \
   || defined(ARDUINO_SAMD_NANO_33_IOT) \
   || defined(ARDUINO_AVR_UNO_WIFI_REV2)
    // Three boards, one branch: their WiFi is the same u-blox NINA-W10
    // co-processor, driven by the same `WiFiNINA` library. What differs —
    // SAMD against megaAVR — is the core, not the radio.
    //
    // Named macros, not `__has_include`: the Arduino build finds libraries by
    // READING the include directives, so a conditional include is never seen
    // and the library never reaches the path. The file says so at length
    // further down, for NimBLE.
    #include "transport/wifi/SoftAP_NINA.hpp"
    #include "transport/wifi/TcpClient_NINA.hpp"
    #include "transport/wifi/TlsClient_NINA.hpp"
    namespace iiot {
        using TransportAP         = SoftAP_NINA;
        using TransportWiFiPlain  = TcpClient_NINA;
        using TransportWiFiSecure = TlsClient_NINA;
    }
    #define INSTANTIOT_HAS_ACCESS_POINT 1
    #define INSTANTIOT_HAS_WIFI_LINK    1
    #define INSTANTIOT_HAS_TLS          1
    // TLS yes, but a root you cannot choose: the trust store is in the module
    // firmware. `TlsClient_NINA` says so when a sketch tries.

#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #include "transport/wifi/SoftAP_ESP8266.hpp"
    #include "transport/wifi/TcpClient_ESP8266.hpp"
    #include "transport/wifi/TlsClient_ESP8266.hpp"
    namespace iiot {
        using TransportAP         = SoftAP_ESP8266;
        using TransportWiFiPlain  = TcpClient_ESP8266;
        using TransportWiFiSecure = TlsClient_ESP8266;
    }
    #define INSTANTIOT_HAS_ACCESS_POINT 1
    #define INSTANTIOT_HAS_WIFI_LINK    1
    #define INSTANTIOT_HAS_TLS          1
    // TLS in software, out of the sketch's own heap — about 20 KB while a
    // session is open, on a chip that has around 40 KB free. It was measured
    // before it was promised; the numbers are in `TlsClient_ESP8266.hpp`,
    // along with the two things that come with them: a receive buffer sized
    // against a server that does not negotiate a smaller one, and a clock,
    // because BearSSL refuses a certificate it cannot date.
#endif

// ── Ethernet — and here the shape of this file changes ──────
//
// Everything above is a chain of `#if/#elif`: one platform, one branch,
// because a board is an ESP32 **or** an R4, never both.
//
// Ethernet is not a platform. It is a LINK, and it coexists with the radio
// on the very board that has one: a Mega with a shield, an ESP32 with a
// W5500 on its SPI bus. Putting it in the chain would have made it exclude
// the WiFi it sits next to.
//
// So it opens its own branch, and that asymmetry is the point.
//
// `INSTANTIOT_ETHERNET` is opt-in rather than detected. `__has_include`
// cannot be used — the Arduino build discovers libraries by READING the
// `#include` directives, and a conditional include is never read, so the
// library never lands on the path. Asking the sketch to say so is the
// honest form: one `#define` before the include.
#if defined(INSTANTIOT_ETHERNET)
  #if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    // Sur ESP32, le W5500 se pilote comme une CARTE RESEAU, derriere lwIP —
    // et le TLS de la plateforme fonctionne alors sans savoir qu'il y a un
    // cable. La bibliotheque `Ethernet` d'Arduino, elle, se sert de la pile
    // TCP cablee dans le composant : rien a envelopper, donc pas de
    // chiffrement possible. Deux facons d'utiliser la meme puce, et une
    // seule ouvre le cloud. Voir l'en-tete d'`EthLink_ESP32.hpp`.
    #include "transport/ethernet/EthLink_ESP32.hpp"
    namespace iiot {
        using TransportEthernet       = EthPlain_ESP32;
        using TransportEthernetSecure = EthTls_ESP32;
    }
    #define INSTANTIOT_HAS_ETHERNET_LINK 1
    #define INSTANTIOT_HAS_ETHERNET_TLS  1
  #else
    #include "transport/ethernet/EthClient_W5x00.hpp"
    namespace iiot { using TransportEthernet = EthClient_W5x00; }
    #define INSTANTIOT_HAS_ETHERNET_LINK 1
  #endif
#endif

// Bluetooth Classic: present in the ESP32 core, absent from the chips
// with no BR/EDR radio. The core's own header refuses to be included in
// that case — so we pose the condition it poses.
#if (defined(ARDUINO_ARCH_ESP32) || defined(ESP32)) \
    && defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
    #include "transport/bluetooth/Bluetooth_ESP32.hpp"
    namespace iiot { using TransportBluetooth = Bluetooth_ESP32; }
    #define INSTANTIOT_HAS_BLUETOOTH 1
#endif

// BLE goes through NimBLE, a library you install rather than part of a
// core.
//
// `__has_include` is NOT enough to find it, and that is a trap in
// Arduino's build system: it discovers libraries by READING `#include`
// directives — it preprocesses, sees an include it cannot resolve, finds
// the library providing it, adds it to the path, and tries again.
// `__has_include` never fails to resolve: it quietly returns 0, so the
// library is never added to the path, so it returns 0. The sketch has to
// break the cycle itself, which also triggers discovery:
//
//     #include <NimBLEDevice.h>
//     #include <InstantIoT.h>
#if (defined(ARDUINO_ARCH_ESP32) || defined(ESP32)) && defined(__has_include)
    #if __has_include(<NimBLEDevice.h>)
        #include "transport/bluetooth/BLE_ESP32.hpp"
        namespace iiot { using TransportBLE = BLE_ESP32; }
        #define INSTANTIOT_HAS_BLE 1
    #endif
#endif

// SoftwareSerial belongs to the AVR core and ships alongside the ESP8266
// one; the ESP32 does not have it at all.
//
// Platform macros, not `__has_include`, for the reason above: the include
// must be SEEN for the library to be added to the search path. With
// `__has_include`, `SerialLink` was unreachable on every board, an Uno
// included.
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #include "transport/serial/SoftSerial.hpp"
    namespace iiot { using TransportSerial = SoftSerial; }
    #define INSTANTIOT_HAS_SERIAL_LINK 1
#endif

namespace iiot {

/**
 * The message for an impossible pairing.
 *
 * Without it, `begin(AccessPoint(…), Cloud(…))` reports "no member named
 * transportTo", which describes my implementation rather than their
 * mistake.
 */
template <class T>
struct AlwaysFalse { static const bool value = false; };

/** The port a board in access-point mode opens. */
#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

// ════════════════════════════════════════════════════════════
//  Direct links — the app is at the other end of the wire
// ════════════════════════════════════════════════════════════

#if defined(INSTANTIOT_HAS_ACCESS_POINT)
/**
 * The board **is** the network: the phone joins its WiFi and talks to it.
 *
 * No server, so no token, no internet, and no replay of the last values —
 * there is nobody to have kept them.
 */
struct AccessPoint {
    const char* ssid;
    const char* pass;
    uint16_t    port;

    AccessPoint(const char* network, const char* password, uint16_t p = INSTANT_AP_PORT)
        : ssid(network), pass(password), port(p) {}

    ITransport& transport() const {
        static TransportAP t(ssid, pass, port);
        return t;
    }

    template <class D>
    ITransport& transportTo(const D&) const {
        static_assert(AlwaysFalse<D>::value,
            "AccessPoint is already the far end: the board IS the network, and "
            "the app connects straight to it. To reach a server, the link is "
            "WiFiLink(ssid, password).");
        return transport();
    }
};
#endif

#if defined(INSTANTIOT_HAS_BLUETOOTH)
/** Bluetooth Classic — the app pairs and talks. */
struct BluetoothLink {
    const char* name;
    explicit BluetoothLink(const char* deviceName = "InstantIoT") : name(deviceName) {}

    ITransport& transport() const {
        static TransportBluetooth t(name);
        return t;
    }
};
#endif

#if defined(INSTANTIOT_HAS_BLE)
/** Bluetooth Low Energy. */
struct BLELink {
    const char* name;
    explicit BLELink(const char* deviceName = "InstantIoT") : name(deviceName) {}

    ITransport& transport() const {
        static TransportBLE t(name);
        return t;
    }
};
#endif

#if defined(INSTANTIOT_HAS_SERIAL_LINK)
/** Two wires. For a board with no radio, or for bring-up. */
struct SerialLink {
    uint8_t rx, tx;
    long    baud;

    SerialLink(uint8_t rxPin, uint8_t txPin, long speed = INSTANT_SERIAL_BAUDRATE)
        : rx(rxPin), tx(txPin), baud(speed) {}

    ITransport& transport() const {
        static TransportSerial t(rx, tx, baud);
        return t;
    }
};
#endif

// ════════════════════════════════════════════════════════════
//  The link that leads elsewhere
// ════════════════════════════════════════════════════════════

#if defined(INSTANTIOT_HAS_WIFI_LINK)
/**
 * The board joins an existing WiFi, then reaches the destination.
 *
 * It knows nothing about the far end: that is what will let
 * `EthernetLink()` take its place without any destination changing.
 */
struct WiFiLink {
    const char* ssid;
    const char* pass;

    WiFiLink(const char* network, const char* password)
        : ssid(network), pass(password) {}

    ITransport& transportTo(const PlainDestination& d) const {
        static TransportWiFiPlain t(d.host, d.port, d.token);
        t.setCredentials(ssid, pass);
        t.setHeartbeat(d.heartbeatMs);
        return t;
    }

#if defined(INSTANTIOT_HAS_TLS)
    ITransport& transportTo(const SecureDestination& d) const {
        static TransportWiFiSecure t(d.host, d.port, d.token);
        t.setCredentials(ssid, pass);
        t.setHeartbeat(d.heartbeatMs);
        // Order matters: a supplied root replaces the embedded ones, and
        // "check nothing" has the last word because it is the more
        // explicit of the two choices.
        if (d.caPem) t.setCACert(d.caPem);
        if (!d.checksIdentity) t.setInsecure();
        return t;
    }
#else
    // A template, not an overload on SecureDestination: an assertion that
    // does not depend on a parameter fires as soon as the class is read,
    // so on every board without TLS — including those that only ever aim
    // for plaintext.
    template <class D>
    ITransport& transportTo(const D& d) const {
        static_assert(AlwaysFalse<D>::value,
            "This board has no TLS stack. The cloud is still reachable in "
            "plaintext — Cloud(TOKEN).plaintext() — and the token then "
            "travels readable on the network.");
        return transportTo(d.plaintext());
    }
#endif
};
#endif

#if defined(INSTANTIOT_HAS_ETHERNET_LINK)
/**
 * A cable, then the destination.
 *
 * The same shape as [WiFiLink] and that is the whole demonstration: a link
 * knows nothing about the far end, so `Cloud(TOKEN)` and `MyServer(host,
 * TOKEN)` work here without one line changing on the destination side.
 *
 *     InstantIoT.begin(EthernetLink(), Cloud(TOKEN));              // ESP32
 *     InstantIoT.begin(EthernetLink(), Cloud(TOKEN).plaintext());  // partout
 *
 * ## Le meme nom, deux montages, et ce n'est pas un caprice
 *
 * Sur ESP32 ce lien pilote le W5500 derriere lwIP, ce qui rend le TLS
 * possible — et demande alors les trois broches, parce qu'un module n'a
 * aucun format impose. Partout ailleurs il passe par la pile TCP cablee
 * dans le composant, ce qui marche sur un Mega et interdit le
 * chiffrement ; l'adresse MAC y est optionnelle, voir
 * [EthClient_W5x00::setMac].
 *
 * La difference n'est pas cosmetique : elle decide de ce que le cable peut
 * porter. L'assertion plus bas la nomme au moment ou elle se voit.
 */
struct EthernetLink {

#if defined(INSTANTIOT_HAS_ETHERNET_TLS)
    // ── ESP32 : le W5500 derriere lwIP, donc le TLS de la plateforme ──
    //
    // Les trois broches n'ont pas de valeur juste : un module W5500 n'a aucun
    // format impose, on le cable. Les defauts ci-dessous sont le cablage le
    // plus repandu, pas une norme — ils existent pour qu'un croquis d'exemple
    // compile. Une mauvaise broche CS echoue bruyamment au demarrage.
    int cs, irq, rst;

    EthernetLink(int csPin = 5, int irqPin = 4, int rstPin = 14)
        : cs(csPin), irq(irqPin), rst(rstPin) {}

    ITransport& transportTo(const PlainDestination& d) const {
        static TransportEthernet t(d.host, d.port, d.token, cs, irq, rst);
        t.setHeartbeat(d.heartbeatMs);
        return t;
    }

    ITransport& transportTo(const SecureDestination& d) const {
        static TransportEthernetSecure t(d.host, d.port, d.token, cs, irq, rst);
        t.setHeartbeat(d.heartbeatMs);
        // Meme ordre que sur le WiFi : une racine fournie remplace les
        // racines embarquees, et « je ne verifie rien » a le dernier mot
        // parce que c'est le plus explicite des deux choix.
        if (d.caPem) t.setCACert(d.caPem);
        if (!d.checksIdentity) t.setInsecure();
        return t;
    }

#else
    // ── Partout ailleurs : la pile TCP cablee dans le W5500 ──
    const uint8_t* mac;

    EthernetLink() : mac(nullptr) {}
    explicit EthernetLink(const uint8_t macAddress[6]) : mac(macAddress) {}

    /**
     * Trois broches : c'est la forme ESP32, et elle n'a pas de sens ici.
     *
     * Sans cette surcharge, un croquis ecrit pour un ESP32 et televerse sur
     * une autre carte reproche une ARITE — « no matching function for call
     * to EthernetLink(int, int, int) » — ce qui envoie corriger un appel
     * alors que c'est la carte qui ne peut pas.
     *
     * Un modele, pour que l'assertion depende d'un parametre et ne se
     * declenche qu'a l'appel reel.
     */
    template <class T>
    EthernetLink(T, T, T) {
        static_assert(AlwaysFalse<T>::value,
            "EthernetLink(cs, irq, rst) is the ESP32 form: there, the W5500 is "
            "driven behind lwIP, the three pins are needed because a module has "
            "no fixed pinout, and TLS works. On this board the cable goes "
            "through the TCP stack wired inside the W5500 — no pins to give, "
            "and no TLS possible. Write EthernetLink() and reach the cloud with "
            "Cloud(TOKEN).plaintext().");
    }

    ITransport& transportTo(const PlainDestination& d) const {
        static TransportEthernet t(d.host, d.port, d.token);
        if (mac) t.setMac(mac);
        t.setHeartbeat(d.heartbeatMs);
        return t;
    }

    /**
     * TLS sur un cable : pas sur CETTE carte, et la raison n'est pas celle
     * qu'on croit.
     *
     * Ce n'est pas que le W5500 « n'a pas de crypto » — le chiffrement
     * tournerait sur le processeur de toute facon. C'est que la
     * bibliotheque `Ethernet` d'Arduino se sert de la pile TCP **cablee dans
     * le composant** : `EthernetClient` est une poignee sur une socket du
     * W5500. Or le TLS des ESP n'enveloppe pas un `Client`, il EN EST un
     * (`class NetworkClientSecure : public NetworkClient`). Il n'y a rien a
     * envelopper.
     *
     * Deux sorties existent, et aucune n'est disponible ici :
     *
     *   · sur ESP32, piloter le W5500 derriere lwIP — c'est la branche
     *     du dessus, et elle marche ;
     *   · sur une carte a puce crypto (MKR, Nano 33 IoT, Uno WiFi Rev.2),
     *     une pile TLS qui se compose sur un `Client` quelconque —
     *     `ArduinoBearSSL` sur un ECCX08. C'est ce que fait Blynk.
     *
     * Sur un AVR sans puce crypto, en revanche, c'est ferme pour de bon : ni
     * la RAM ni la flash pour une poignee de main, et rien pour la faire a
     * sa place. Mesure : la bibliotheque seule prend deja 61 % de la SRAM
     * d'un Uno.
     *
     * Un modele plutot qu'une surcharge sur `SecureDestination`, pour la
     * meme raison que [WiFiLink] : une assertion qui ne depend d'aucun
     * parametre se declenche des que la classe est lue, donc sur tous les
     * croquis Ethernet — y compris ceux qui ne visaient que le clair.
     */
    template <class D>
    ITransport& transportTo(const D& d) const {
        static_assert(AlwaysFalse<D>::value,
            "No TLS over Ethernet on this board. Arduino's Ethernet library "
            "uses the TCP stack wired INSIDE the W5500, and a TLS client "
            "cannot wrap one of its sockets. On an ESP32 the same chip can be "
            "driven behind lwIP instead, and then TLS works. On an AVR it "
            "cannot: no crypto co-processor, and no RAM for a handshake. "
            "The cloud stays reachable in plaintext — Cloud(TOKEN).plaintext() "
            "— and the token then travels readable on the network.");
        return transportTo(d.plaintext());
    }
#endif

};
#endif

// ════════════════════════════════════════════════════════════
//  The links this board does not have
// ════════════════════════════════════════════════════════════
//
// Without these shells, `WiFiLink` on an ESP8266 reports "was not
// declared in this scope; did you mean 'WiFiClient'?" — and the user goes
// off correcting a typo they never made. The shell exists, so the name
// resolves, and the message states the real reason.
//
// `sizeof...(A) < 0` is always false and depends on a template
// parameter: the assertion is only evaluated if the sketch really builds
// this link.
#define _IIO_LINK_ABSENT(NAME, WHY)                                        \
    struct NAME {                                                          \
        template <class... A>                                              \
        explicit NAME(A&&...) { static_assert(sizeof...(A) < 0, WHY); }    \
        ITransport& transport() const;                                     \
    };

#if !defined(INSTANTIOT_HAS_ETHERNET_LINK)
_IIO_LINK_ABSENT(EthernetLink,
    "EthernetLink needs the Ethernet library, and the sketch has to ask for "
    "it by name: put `#define INSTANTIOT_ETHERNET 1` BEFORE "
    "`#include <InstantIoT.h>`. It is not detected on its own — the Arduino "
    "build finds libraries by reading #include directives, so a conditional "
    "include is never seen and the library never reaches the path.")
#endif

#if !defined(INSTANTIOT_HAS_ACCESS_POINT)
_IIO_LINK_ABSENT(AccessPoint,
    "InstantIoT has no access point for this board. There is one for ESP32, "
    "ESP8266 and Uno R4 WiFi; adding one for yours is an ITransport in "
    "src/transport/wifi/ and a branch in Links.hpp.")
#endif

#if !defined(INSTANTIOT_HAS_WIFI_LINK)
_IIO_LINK_ABSENT(WiFiLink,
    "InstantIoT cannot join an existing WiFi from this board. On the ESP8266, "
    "which only has the access point, write AccessPoint(name, password) "
    "instead: the phone joins the board's own WiFi. On a board that has not "
    "been ported, it is an ITransport to write and a branch to add in "
    "Links.hpp.")
#endif

#if !defined(INSTANTIOT_HAS_BLUETOOTH)
_IIO_LINK_ABSENT(BluetoothLink,
    "InstantIoT only has Bluetooth Classic for the ESP32, and only those with "
    "a BR/EDR radio — not the -S2 nor the -C3.")
#endif

#if !defined(INSTANTIOT_HAS_BLE)
_IIO_LINK_ABSENT(BLELink,
    "BLE needs an ESP32 and the NimBLE-Arduino library. If it is installed, "
    "the sketch must include it ITSELF before InstantIoT.h — "
    "#include <NimBLEDevice.h> — otherwise the Arduino compiler does not add "
    "it to the search path.")
#endif

#if !defined(INSTANTIOT_HAS_SERIAL_LINK)
_IIO_LINK_ABSENT(SerialLink,
    "SoftwareSerial does not exist on this board. It is in the AVR core and "
    "in the ESP8266 one; neither the ESP32, nor the Uno R4 WiFi, nor the SAMD "
    "core has it.")
#endif

}  // namespace iiot
