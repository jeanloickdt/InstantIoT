#pragma once
/**
 * ============================================================
 * 📻 RaisonWiFi_ESP32.hpp — pourquoi l'association a échoué
 * ============================================================
 *
 * ## Ce que ce fichier a coûté avant d'exister
 *
 * Une carte qui s'était toujours connectée a cessé de le faire. Le
 * croquis disait « pas joint », la lib disait « WiFi timeout », et il a
 * fallu une heure, quatre croquis de diagnostic et deux correctifs
 * inutiles pour découvrir que le mot de passe avait perdu deux
 * caractères dans un copier-coller.
 *
 * L'ESP32, lui, le savait **dès la première seconde**. Il émet un
 * événement de déconnexion avec un code de raison, et ce code disait
 * `15 — 4WAY_HANDSHAKE_TIMEOUT`, qui veut dire « la clé est refusée ».
 * Personne ne le lui avait demandé.
 *
 * ## Pourquoi ça parle sans qu'on le lui demande
 *
 * Le reste des journaux de la lib est compilé hors du binaire tant que
 * `INSTANTIOT_DEBUG` vaut 0, et c'est bien : un journal bavard coûte du
 * flash et du temps pour rien.
 *
 * Une association refusée n'est pas « pour rien ». C'est le seul moment
 * où la carte ne peut RIEN faire d'autre, et où celui qui la regarde n'a
 * aucun moyen de savoir pourquoi. Alors elle le dit — une fois par
 * raison, pas à chaque tentative, sans quoi le backoff transformerait le
 * moniteur en cascade.
 *
 * `#define INSTANTIOT_QUIET 1` la fait taire, pour un produit fini qui
 * a d'autres façons de se diagnostiquer.
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "RaisonWiFi_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>

namespace iiot {

/** Le dernier code de raison reçu, et celui déjà annoncé. */
inline uint8_t& derniereRaisonWiFi() { static uint8_t r = 0; return r; }
inline uint8_t& raisonWiFiDeja()     { static uint8_t r = 0; return r; }

/**
 * Les raisons qu'on rencontre vraiment, en français et sans jargon.
 *
 * La liste complète vit dans `esp_wifi_types.h` et compte une trentaine
 * d'entrées ; les embarquer toutes coûterait du flash pour des cas que
 * personne ne voit. Celles-ci couvrent ce qui arrive sur un établi.
 */
inline const char* texteRaisonWiFi(uint8_t r) {
    switch (r) {
        case 15:  return "cle refusee par le point d'acces (mot de passe)";
        case 2:   return "authentification expiree";
        case 201: return "point d'acces introuvable (nom, bande 2,4 GHz, portee)";
        case 202: return "authentification refusee (WPA3 seul ? filtrage MAC ?)";
        case 203: return "association refusee (trop de clients ?)";
        case 204: return "poignee de main trop longue";
        case 205: return "connexion echouee";
        default:  return "voir esp_wifi_types.h";
    }
}

/** Retient la raison. Ne l'affiche pas : un gestionnaire d'événement est
 *  appelé depuis la pile WiFi, et on n'y écrit pas sur le port série. */
inline void noteLaRaisonWiFi(WiFiEvent_t evenement, WiFiEventInfo_t info) {
    if (evenement == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
        derniereRaisonWiFi() = info.wifi_sta_disconnected.reason;
}

/** À appeler une fois, avant la première tentative. */
inline void ecouteLesRaisonsWiFi() {
    static bool inscrit = false;
    if (inscrit) return;
    inscrit = true;
    WiFi.onEvent(noteLaRaisonWiFi);
}

/**
 * Dit la raison, si elle est nouvelle. Appelé depuis `loop()`, jamais
 * depuis l'événement.
 */
inline void diLaRaisonWiFi() {
#ifndef INSTANTIOT_QUIET
    uint8_t r = derniereRaisonWiFi();
    if (r == 0 || r == raisonWiFiDeja()) return;
    raisonWiFiDeja() = r;
    Serial.print(F("[InstantIoT] WiFi refuse — raison "));
    Serial.print(r);
    Serial.print(F(" : "));
    Serial.println(texteRaisonWiFi(r));
#endif
}

/** Après une association reussie : la prochaine raison sera neuve. */
inline void oublieLaRaisonWiFi() {
    derniereRaisonWiFi() = 0;
    raisonWiFiDeja()     = 0;
}

} // namespace iiot
