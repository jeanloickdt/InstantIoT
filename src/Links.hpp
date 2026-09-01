#pragma once
/**
 * ============================================================
 * 🔌 Links.hpp — comment la carte atteint le réseau
 * ============================================================
 *
 * Une liaison est le **chemin**, une destination est le **bout**. Le
 * `begin()` se lit dans l'ordre où les choses se passent : la carte rejoint
 * d'abord un réseau, puis atteint un serveur.
 *
 *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), Cloud(TOKEN));
 *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), MyMyServer("192.168.1.42", TOKEN));
 *
 * Trois liaisons n'ont pas de bout à nommer : l'app est déjà au bout du
 * fil. Elles se passent seules, et le premier argument ne change pas.
 *
 *     InstantIoT.begin(AccessPoint("MaCarte", "12345678"));
 *     InstantIoT.begin(BluetoothLink("MaCarte"));
 *     InstantIoT.begin(SerialLink(10, 11));
 *
 * ## Le suffixe
 *
 * `WiFiLink` et non `WiFi` : le cœur Arduino a déjà un objet global nommé
 * `WiFi`, et deux choses ne portent pas le même nom. Le suffixe dit aussi
 * ce que c'est — un chemin, pas une destination — ce qui est exactement la
 * distinction que ce fichier existe pour tenir.
 *
 * ## Ethernet
 *
 * Le modèle l'attend : `InstantIoT.begin(EthernetLink(), Cloud(TOKEN))`
 * n'aurait rien à changer ailleurs, puisque la destination ne sait pas par
 * où on l'atteint. Le transport, lui, n'existe pas encore — il n'est pas
 * écrit ici tant qu'il n'a pas tourné sur une carte.
 *
 * ## Ce que chaque carte sait faire
 *
 * ESP32       AccessPoint, WiFiLink (clair et TLS), BluetoothLink, BLELink
 * Uno R4 WiFi AccessPoint, WiFiLink (clair et TLS)
 * ESP8266     AccessPoint
 *
 * Un couple que la carte ne sait pas faire ne compile pas, et le dit en
 * une phrase plutôt qu'en une page de gabarits.
 * ============================================================
 */

#include "Destinations.hpp"
#include "core/Transport.h"

// ════════════════════════════════════════════════════════════
//  Ce que la plateforme apporte
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

#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #include "transport/wifi/SoftAP_ESP8266.hpp"
    namespace iiot { using TransportAP = SoftAP_ESP8266; }
    #define INSTANTIOT_HAS_ACCESS_POINT 1
#endif

// Bluetooth classique : présent dans le cœur ESP32, absent des puces qui
// n'ont pas de radio BR/EDR. L'en-tete du cœur refuse d'etre inclus dans ce
// cas — c'est donc la condition qu'il pose que l'on pose ici.
#if (defined(ARDUINO_ARCH_ESP32) || defined(ESP32)) \
    && defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
    #include "transport/bluetooth/Bluetooth_ESP32.hpp"
    namespace iiot { using TransportBluetooth = Bluetooth_ESP32; }
    #define INSTANTIOT_HAS_BLUETOOTH 1
#endif

// BLE passe par NimBLE, qui est une bibliotheque a installer et non une
// partie du cœur. On ne la reclame que si elle est la.
#if (defined(ARDUINO_ARCH_ESP32) || defined(ESP32)) && defined(__has_include)
    #if __has_include(<NimBLEDevice.h>)
        #include "transport/bluetooth/BLE_ESP32.hpp"
        namespace iiot { using TransportBLE = BLE_ESP32; }
        #define INSTANTIOT_HAS_BLE 1
    #endif
#endif

// SoftwareSerial appartient au cœur AVR et se pose a cote sur ESP8266 ;
// l'ESP32 ne l'a pas du tout.
#if defined(__has_include)
    #if __has_include(<SoftwareSerial.h>) && !defined(ESP32)
        #include "transport/serial/SoftSerial.hpp"
        namespace iiot { using TransportSerial = SoftSerial; }
        #define INSTANTIOT_HAS_SERIAL_LINK 1
    #endif
#endif

namespace iiot {

/**
 * Le message d'un couple impossible.
 *
 * Sans lui, `begin(AccessPoint(…), Cloud(…))` sort « aucun membre nommé
 * transportVers », ce qui décrit ma mise en œuvre et non son erreur.
 */
template <class T>
struct AlwaysFalse { static const bool value = false; };

/** Le port du serveur qu'une carte en point d'acces ouvre. */
#ifndef INSTANT_AP_PORT
  #define INSTANT_AP_PORT 8080
#endif

// ════════════════════════════════════════════════════════════
//  Les liaisons directes — l'app est au bout du fil
// ════════════════════════════════════════════════════════════

#if defined(INSTANTIOT_HAS_ACCESS_POINT)
/**
 * La carte **est** le réseau : le téléphone rejoint son WiFi et lui parle.
 *
 * Pas de serveur, donc pas de jeton, pas d'internet, et pas de rappel des
 * dernières valeurs — il n'y a personne pour les avoir gardées.
 */
struct AccessPoint {
    const char* ssid;
    const char* pass;
    uint16_t    port;

    AccessPoint(const char* reseau, const char* motDePasse, uint16_t p = INSTANT_AP_PORT)
        : ssid(reseau), pass(motDePasse), port(p) {}

    ITransport& transport() const {
        static TransportAP t(ssid, pass, port);
        return t;
    }

    template <class D>
    ITransport& transportVers(const D&) const {
        static_assert(AlwaysFalse<D>::value,
            "AccessPoint est deja le bout du fil : la carte EST le reseau, et "
            "l'app s'y connecte directement. Pour atteindre un serveur, la "
            "liaison est WiFiLink(ssid, mot_de_passe).");
        return transport();
    }
};
#endif

#if defined(INSTANTIOT_HAS_BLUETOOTH)
/** Bluetooth classique — l'app s'appaire et parle. */
struct BluetoothLink {
    const char* name;
    explicit BluetoothLink(const char* nom = "InstantIoT") : name(nom) {}

    ITransport& transport() const {
        static TransportBluetooth t(name);
        return t;
    }
};
#endif

#if defined(INSTANTIOT_HAS_BLE)
/** Bluetooth basse consommation. */
struct BLELink {
    const char* name;
    explicit BLELink(const char* nom = "InstantIoT") : name(nom) {}

    ITransport& transport() const {
        static TransportBLE t(name);
        return t;
    }
};
#endif

#if defined(INSTANTIOT_HAS_SERIAL_LINK)
/** Deux fils. Pour une carte sans radio, ou pour mettre au point. */
struct SerialLink {
    uint8_t rx, tx;
    long    baud;

    SerialLink(uint8_t brocheRx, uint8_t brocheTx, long vitesse = INSTANT_SERIAL_BAUDRATE)
        : rx(brocheRx), tx(brocheTx), baud(vitesse) {}

    ITransport& transport() const {
        static TransportSerial t(rx, tx, baud);
        return t;
    }
};
#endif

// ════════════════════════════════════════════════════════════
//  La liaison qui mène ailleurs
// ════════════════════════════════════════════════════════════

#if defined(INSTANTIOT_HAS_WIFI_LINK)
/**
 * La carte rejoint un WiFi existant, puis atteint la destination.
 *
 * Elle ne sait rien du bout : c'est ce qui permettra à `EthernetLink()` de
 * prendre sa place sans qu'aucune destination ne change.
 */
struct WiFiLink {
    const char* ssid;
    const char* pass;

    WiFiLink(const char* reseau, const char* motDePasse)
        : ssid(reseau), pass(motDePasse) {}

    ITransport& transportVers(const PlainDestination& d) const {
        static TransportWiFiPlain t(d.host, d.port, d.token);
        t.setCredentials(ssid, pass);
        t.setHeartbeat(d.heartbeatMs);
        return t;
    }

#if defined(INSTANTIOT_HAS_TLS)
    ITransport& transportVers(const SecureDestination& d) const {
        static TransportWiFiSecure t(d.host, d.port, d.token);
        t.setCredentials(ssid, pass);
        t.setHeartbeat(d.heartbeatMs);
        // L'ordre compte : une racine fournie remplace les racines
        // embarquees, et « ne rien verifier » a le dernier mot parce que
        // c'est le choix le plus explicite des deux.
        if (d.caPem) t.setCACert(d.caPem);
        if (!d.checksIdentity) t.setInsecure();
        return t;
    }
#else
    // Gabarit, et non surcharge sur SecureDestination : une assertion qui
    // ne depend pas d'un parametre se declenche des la lecture de la classe,
    // donc sur toute carte sans TLS, meme celles qui ne visent que le clair.
    template <class D>
    ITransport& transportVers(const D& d) const {
        static_assert(AlwaysFalse<D>::value,
            "Cette carte n'a pas de pile TLS. Le cloud reste atteignable en "
            "clair — Cloud(TOKEN).plaintext() — et le jeton passe alors "
            "lisible sur le reseau.");
        return transportVers(d.plaintext());
    }
#endif
};
#endif

}  // namespace iiot
