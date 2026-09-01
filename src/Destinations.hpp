#pragma once
/**
 * ============================================================
 * 🎯 Destinations.hpp — à qui la carte parle
 * ============================================================
 *
 * Une destination est une **description**, pas un objet vivant : un hôte,
 * un port, un jeton, et la façon de vérifier qu'on parle bien à qui on
 * croit. Elle se lit dans le `begin()` et n'y survit pas — c'est la façade
 * qui en tire le transport et le garde.
 *
 *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), Cloud(TOKEN));
 *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), MyServer("192.168.1.42", TOKEN));
 *
 * ## Pourquoi deux types et non un drapeau
 *
 * `Cloud(TOKEN)` chiffre ; `Cloud(TOKEN).plaintext()` ne chiffre pas. La
 * différence n'est pas un booléen à l'exécution : elle change le type, donc
 * le transport, donc ce que l'éditeur de liens embarque. Une carte sans
 * pile TLS — et il en existe, et leur propriétaire le sait — peut alors
 * atteindre le cloud pour de bon, au lieu de refuser de compiler à cause
 * d'un chemin qu'elle n'emprunte jamais.
 *
 * En clair veut dire en clair : le jeton et les valeurs passent lisibles.
 * C'est un choix qui se prend en connaissance de cause, pas un défaut.
 * ============================================================
 */

#include <stdint.h>

/** L'hôte du cloud InstantIoT. Redéfinissable pour une préproduction. */
#ifndef INSTANTIOT_CLOUD_HOST
  #define INSTANTIOT_CLOUD_HOST "instantiot.cloud"
#endif

/** Portier TLS. */
#ifndef INSTANTIOT_CLOUD_TLS_PORT
  #define INSTANTIOT_CLOUD_TLS_PORT 9443
#endif

/** Portier en clair — le même que celui d'un serveur auto-hébergé. */
#ifndef INSTANTIOT_CLOUD_PLAIN_PORT
  #define INSTANTIOT_CLOUD_PLAIN_PORT 9001
#endif

/** Le battement par défaut : le serveur cale son délai d'absence dessus. */
#ifndef INSTANTIOT_DEFAULT_HEARTBEAT_MS
  #define INSTANTIOT_DEFAULT_HEARTBEAT_MS 5000
#endif

namespace iiot {

struct SecureDestination;

/**
 * Une destination en clair — TCP nu.
 *
 * C'est le cas normal d'un serveur auto-hébergé sur son propre réseau, où
 * le chiffrement protégerait un trajet qui ne sort pas de la maison. C'est
 * aussi le seul cas possible pour une carte sans pile TLS.
 */
struct PlainDestination {
    const char* host;
    uint16_t    port;
    const char* token;
    uint32_t    heartbeatMs = INSTANTIOT_DEFAULT_HEARTBEAT_MS;

    PlainDestination(const char* h, uint16_t p, const char* t)
        : host(h), port(p), token(t) {}

    /** Le rythme du battement de présence, en millisecondes. 0 le coupe. */
    PlainDestination& heartbeatEvery(uint32_t ms) { heartbeatMs = ms; return *this; }

    /** Passe au chiffré, sur le même hôte et le même port. */
    SecureDestination secure() const;
};

/**
 * Une destination chiffrée — TLS.
 *
 * L'identité du serveur est vérifiée par défaut, contre les racines
 * Let's Encrypt embarquées. Deux façons d'en sortir, et elles ne disent
 * pas la même chose :
 *
 *   `.withCertificate(pem)` — « voici ma racine à moi ». L'identité reste
 *   vérifiée, contre une autorité que vous fournissez. C'est ce qu'il faut
 *   pour un serveur auto-hébergé avec un certificat maison.
 *
 *   `.withoutCertCheck()` — « je ne vérifie rien ». Le trajet reste
 *   chiffré, mais n'importe qui peut se faire passer pour le serveur. Pour
 *   un premier démarrage, pas pour ce qui reste branché.
 */
struct SecureDestination {
    const char* host;
    uint16_t    port;
    const char* token;
    /** Nul = les racines embarquées. */
    const char* caPem = nullptr;
    bool        checksIdentity = true;
    uint32_t    heartbeatMs = INSTANTIOT_DEFAULT_HEARTBEAT_MS;
    /** Vrai dès que le croquis a nommé un port : `plaintext()` le respecte. */
    bool        portWasChosen = false;

    SecureDestination(const char* h, uint16_t p, const char* t)
        : host(h), port(p), token(t) {}

    SecureDestination& withCertificate(const char* pem) { caPem = pem; return *this; }
    SecureDestination& withoutCertCheck() { checksIdentity = false; return *this; }
    SecureDestination& heartbeatEvery(uint32_t ms) { heartbeatMs = ms; return *this; }

    /** Une préproduction, un cloud auto-hébergé — le reste ne bouge pas. */
    SecureDestination& at(const char* h, uint16_t p) {
        host = h; port = p; portWasChosen = true; return *this;
    }

    /**
     * Passe en clair.
     *
     * Le port suit : si le croquis n'en a pas nommé, celui du portier TLS
     * n'a plus de sens et cède la place au portier en clair. S'il en a
     * nommé un, il est respecté — c'est lui qui savait.
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
//  Les deux noms que le croquis écrit
// ════════════════════════════════════════════════════════════

/** Le cloud InstantIoT. Chiffré, identité vérifiée. */
inline SecureDestination Cloud(const char* token) {
    return SecureDestination(INSTANTIOT_CLOUD_HOST, INSTANTIOT_CLOUD_TLS_PORT, token);
}

/**
 * Un serveur que vous hébergez. En clair, port 9001 par défaut.
 *
 * `MyServer` et non `Server` : le cœur Arduino a déjà une `class Server`,
 * et le compilateur refuse le nom nu — la même collision que `WiFi`. Le
 * possessif dit d'ailleurs la vraie différence avec `Cloud` : hébergé par
 * nous, ou hébergé par vous.
 */
inline PlainDestination MyServer(const char* host, const char* token) {
    return PlainDestination(host, INSTANTIOT_CLOUD_PLAIN_PORT, token);
}

inline PlainDestination MyServer(const char* host, uint16_t port, const char* token) {
    return PlainDestination(host, port, token);
}

}  // namespace iiot
