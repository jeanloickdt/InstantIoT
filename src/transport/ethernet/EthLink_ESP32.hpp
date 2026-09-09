#pragma once
/**
 * ============================================================
 * 🔗 EthLink_ESP32.hpp — le câble, mais derrière lwIP
 * ============================================================
 *
 * Le même W5500 que [EthClient_W5x00], et pourtant l'autre bout du monde.
 *
 * ## Deux façons d'utiliser la même puce, et une seule permet le TLS
 *
 * La bibliothèque `Ethernet` d'Arduino se sert de la pile TCP/IP **câblée
 * dans le W5500** : `EthernetClient` est une poignée sur une socket du
 * composant. C'est ce qui la rend utilisable sur un Mega qui n'a pas la RAM
 * d'une pile logicielle — et c'est aussi ce qui interdit le chiffrement.
 *
 * Le TLS des ESP n'enveloppe pas un `Client`, il EN EST un :
 *
 *     class NetworkClientSecure : public NetworkClient   // cœur 3.3.7
 *
 * Il hérite d'une socket lwIP. Une socket du W5500 n'en est pas une, et il
 * n'y a donc rien à envelopper. Ce n'est pas une question de crypto absente
 * du W5500 : le chiffrement tournerait sur l'ESP32 de toute façon.
 *
 * Ce fichier prend l'autre chemin. Le cœur ESP32 sait piloter un W5500
 * comme une **carte réseau** — `ETH.begin(ETH_PHY_W5500, …)` — et le place
 * alors derrière lwIP, exactement comme le WiFi. À partir de là,
 * `NetworkClientSecure` fonctionne sans rien savoir du câble, et
 * `Cloud(TOKEN)` chiffré devient possible sur Ethernet.
 *
 *     ETHClass : public NetworkInterface
 *
 * ## Ce que le croquis doit dire, et pourquoi il ne peut pas se taire
 *
 * Un shield Ethernet au format Uno a un brochage imposé : sa broche CS est
 * la 10, et `EthernetLink()` n'a rien à demander. Un module W5500 sur un
 * ESP32 n'a aucun format — on le câble. Les trois broches sont donc une
 * information que seule la personne qui a soudé possède.
 *
 * Les valeurs par défaut ci-dessous sont le câblage le plus répandu, pas une
 * norme. Elles existent pour qu'un croquis d'exemple compile ; changez-les
 * pour le vôtre. Une mauvaise broche CS donne « no hardware found » au
 * démarrage, ce qui est un échec bruyant — contrairement à une adresse MAC
 * en double, qui se paie bien plus tard.
 *
 * Le bus SPI lui-même n'est pas configuré ici : si vos broches SCK/MISO/MOSI
 * ne sont pas celles par défaut de la carte, appelez `SPI.begin(sck, miso,
 * mosi)` dans `setup()` AVANT `InstantIoT.begin(...)`.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "EthLink_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <NetworkClientSecure.h>
#include "../TcpSession.hpp"
#include "../../certs/InstantIoT_LE_Roots.h"

/** Borne la négociation TLS, en secondes — comme sur le WiFi. */
#ifndef INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S
  #define INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S 10
#endif

/**
 * L'adresse du PHY sur le bus. 1 pour un W5500, et ce n'est pas un réglage :
 * c'est ce que le pilote du cœur attend pour ce composant.
 */
#ifndef INSTANTIOT_ETH_PHY_ADDR
  #define INSTANTIOT_ETH_PHY_ADDR 1
#endif

namespace iiot {

/**
 * Le lien — la partie qui ne dépend pas du chiffrement.
 *
 * Les deux transports qui suivent n'en diffèrent que par le client qu'ils
 * portent : un `NetworkClient` en clair, un `NetworkClientSecure` chiffré.
 * Le câble, lui, se lève de la même façon.
 */
class EthLink_ESP32 : public TcpSession {
public:
    /** Le câble est-il branché ET avons-nous une adresse ? */
    IPAddress getLocalIP() const { return ETH.localIP(); }

protected:

    EthLink_ESP32(
        Client&     client,
        const char* serverIp,
        uint16_t    serverPort,
        const char* token,
        int cs, int irq, int rst
    ) : TcpSession(client, serverIp, serverPort, token)
      , cs_(cs), irq_(irq), rst_(rst)
    {}

    /**
     * `linkUp()` ne suffit pas : il répond sur le CÂBLE.
     *
     * Le DHCP arrive après, par un événement, et ouvrir une session TLS avec
     * 0.0.0.0 échoue sans rien expliquer. Le même piège que l'Uno R4, dont
     * le modem annonce la connexion avant la fin du bail.
     */
    bool linkUp() const override {
        return ETH.linkUp() && ETH.localIP() != IPAddress(0, 0, 0, 0);
    }

    /**
     * Démarre le pilote, UNE FOIS.
     *
     * @return true : rien n'a bloqué. Contrairement au `Ethernet.begin(mac)`
     *         de la bibliothèque Arduino, qui reste dans le DHCP jusqu'à
     *         obtenir un bail ou expirer, celui-ci rend la main aussitôt et
     *         le bail arrive par un événement. C'est donc une tentative EN
     *         VOL, que [TcpSession] doit surveiller sans y toucher.
     *
     * Le garde n'est pas de la prudence : `ETH.begin` réinitialise le
     * pilote. L'appeler à chaque tentative de reconnexion couperait le câble
     * qu'on essaie de rétablir.
     */
    bool beginLink() override {
        if (!demarre_) {
            IIOT_LOG("[ETH] Starting the W5500 driver");
            if (!ETH.begin(ETH_PHY_W5500, INSTANTIOT_ETH_PHY_ADDR,
                           cs_, irq_, rst_, SPI)) {
                IIOT_LOG("[ETH] Driver refused to start — check CS/IRQ/RST and the SPI wiring");
                return false;   // rien en vol : le backoff doit s'armer
            }
            demarre_ = true;
        }
        // Le pilote tourne ; le câble et le bail arrivent quand ils
        // arrivent. Il y a bien quelque chose à attendre.
        return true;
    }

private:
    int  cs_, irq_, rst_;
    bool demarre_ = false;
};

/**
 * Ethernet en clair — pour un serveur que vous hébergez.
 *
 * Le membre est déclaré APRÈS la base, qui n'en garde qu'une référence : le
 * même arrangement que tous les autres transports, et la note en tête de
 * `TcpClient_ESP32` explique pourquoi il tient.
 */
class EthPlain_ESP32 : public EthLink_ESP32 {
public:
    EthPlain_ESP32(
        const char* serverIp, uint16_t serverPort, const char* token,
        int cs, int irq, int rst
    ) : EthLink_ESP32(netClient_, serverIp, serverPort, token, cs, irq, rst) {}

protected:
    /** Le même réglage que sur le WiFi : la voie de commande n'attend pas. */
    void prepareClient() override {
        netClient_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);
        netClient_.setNoDelay(true);
    }

private:
    NetworkClient netClient_;
};

/**
 * Ethernet chiffré — et c'est tout l'objet de ce fichier.
 *
 * Rien ici ne parle du câble : `NetworkClientSecure` ne sait pas par où
 * passent ses octets, et c'est exactement ce qu'on est venu chercher. La
 * confiance se règle comme sur le WiFi, avec les mêmes racines embarquées.
 */
class EthTls_ESP32 : public EthLink_ESP32 {
public:
    EthTls_ESP32(
        const char* serverIp, uint16_t serverPort, const char* token,
        int cs, int irq, int rst
    ) : EthLink_ESP32(tlsClient_, serverIp, serverPort, token, cs, irq, rst)
      , caCert_(INSTANTIOT_LE_ROOT_CAS)
      , insecure_(false)
    {}

    /** Votre propre racine, pour un serveur avec son autorité. */
    void setCACert(const char* pem) { caCert_ = pem; insecure_ = false; }

    /** Chiffre sans vérifier l'identité. Pour une mise en route, pas après. */
    void setInsecure() { insecure_ = true; }

protected:
    /**
     * Réglé à CHAQUE tentative, comme sur le WiFi : une reconnexion
     * reconstruit le contexte, et traiter ces appels comme uniques est la
     * façon dont un transport se rebranche sur une session non vérifiée.
     */
    void prepareClient() override {
        if (insecure_) {
            tlsClient_.setInsecure();
            IIOT_LOG("[ETH-TLS] ⚠️ INSECURE — the server's identity is NOT verified");
        } else {
            tlsClient_.setCACert(caCert_);
        }
        tlsClient_.setHandshakeTimeout(INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S);
        tlsClient_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);
    }

private:
    const char* caCert_;
    bool        insecure_;
    NetworkClientSecure tlsClient_;
};

} // namespace iiot
