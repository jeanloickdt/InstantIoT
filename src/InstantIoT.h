#pragma once
/**
 * ============================================================
 * ⚡ InstantIoT.h — l'objet unique du croquis
 * ============================================================
 *
 * ## Ce qu'il remplace
 *
 * Chaque croquis déclarait sa façade en global :
 *
 *     InstantIoTWiFiAP instant("MaCarte", "12345678");   // effacee au lot B
 *     void setup() { instant.begin(); }
 *     void loop()  { instant.loop();  }
 *
 * Et `instant` n'était atteignable que parce que le croquis avait pensé à
 * la déclarer **hors** de `setup()`. Déclarée dedans, aucune fonction, aucun
 * bloc `ISignal` ne pouvait plus écrire — et rien dans la lib ne le disait.
 * Le nom changeait aussi avec le mode de liaison, si bien qu'un exemple ne
 * se copiait pas d'un mode à l'autre.
 *
 *     void setup() { InstantIoT.begin(lien); }
 *     void loop()  { InstantIoT.loop();      }
 *
 * ## Appelable où ?
 *
 * Partout, et c'est le sujet. Depuis `loop()`, depuis une fonction à vous,
 * depuis un bloc `ISignal` ou `ISimpleButton` — répondre à un geste par une
 * écriture est le cas normal, et la lecture et l'écriture ne se disputent
 * pas de tampon (`_rxBuffer` d'un côté, `_txBuffer` de l'autre).
 *
 * Deux réserves, et elles sont réelles :
 *
 * **Avant `begin()`**, il n'y a pas de liaison. L'appel ne fait rien et le
 * dit une fois au journal. Il ne déréférence rien : les champs de cet objet
 * sont initialisés à la compilation, donc valides même pendant la
 * construction des globales des autres unités. C'est ce qu'un global
 * ordinaire ne garantit pas, et pourquoi ils n'ont pas de constructeur.
 *
 * **Depuis une interruption**, non — écrire sur une socket depuis un ISR
 * n'est sûr avec aucune bibliothèque. La règle Arduino ne change pas :
 * l'interruption pose un drapeau, `loop()` écrit.
 * ============================================================
 */

#include "core/InstantIoTCore.hpp"

/**
 * Le DSL vient avec.
 *
 * Aucun en-tête ne tirait `InstantIoTWhen.hpp` : un croquis n'avait
 * `ISignal` que s'il pensait à l'inclure lui-même, et aucun des vingt-cinq
 * exemples ne le faisait — ils ne compilent plus depuis que le DSL s'est
 * rebâti sur les signaux. Un seul `#include <InstantIoT.h>` doit suffire
 * pour tout ce qu'un croquis écrit.
 */
#include "utils/InstantIoTWhen.hpp"

/** Les liaisons et les destinations que `begin()` accepte. */
#include "Links.hpp"

/** Les minuteries — `timers.every(1000, publier)` plutôt qu'un `millis()`. */
#include "utils/InstantIoTTimer.hpp"

namespace iiot {

/**
 * La façade. Un seul exemplaire, nommé `InstantIoT`, déclaré plus bas.
 *
 * Elle ne fait pas le travail : elle tient le cœur et lui passe la main.
 * Le cœur n'existe qu'à partir de `begin()`, parce que c'est là seulement
 * qu'on sait par quelle liaison la carte parle.
 */
class Facade {
public:

    /**
     * Ouvre une liaison directe — l'app est au bout du fil.
     *
     *     InstantIoT.begin(AccessPoint("MaCarte", "12345678"));
     *     InstantIoT.begin(BluetoothLink("MaCarte"));
     *
     * La liaison passée est une description : elle ne survit pas à la
     * ligne, et c'est la façade qui garde le transport qu'elle en tire.
     *
     * Le `decltype` n'est pas de la coquetterie : sans lui, cette surcharge
     * prendrait aussi les transports bruts, qui n'ont pas de `transport()`,
     * et le message parlerait de gabarits au lieu de dire quoi corriger.
     */
    template <class Link>
    auto begin(const Link& lien) -> decltype(lien.transport(), bool()) {
        return begin(lien.transport());
    }

    /**
     * Ouvre une liaison vers une destination.
     *
     *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), Cloud(TOKEN));
     *     InstantIoT.begin(WiFiLink("MonWiFi", "secret"), MyMyServer("192.168.1.42", TOKEN));
     *
     * La liaison ne sait pas ce qu'est le bout, et la destination ne sait
     * pas par où on l'atteint. C'est ce qui permettra à une liaison de plus
     * — Ethernet, demain — de n'obliger aucune destination à changer.
     */
    template <class Link, class Dest>
    bool begin(const Link& lien, const Dest& dest) {
        const bool premier = (_coeur == nullptr);
        const bool ouverte = begin(lien.transportVers(dest));
        // Le battement est plombé sur les deux couches : le transport
        // l'annonce au serveur à la poignee de main, le cœur l'émet. Pas
        // sur un second `begin()`, qui ne rebâtit rien.
        if (premier) setHeartbeat(dest.heartbeatMs);
        return ouverte;
    }

    /**
     * Ouvre une liaison déjà construite.
     *
     * L'échappatoire : un transport à vous, ou un des transports de la lib
     * instancié à la main. Il doit vivre aussi longtemps que le programme —
     * une globale ou un `static`, jamais un temporaire.
     *
     * @return vrai si la liaison s'est ouverte.
     */
    bool begin(ITransport& lien) {
        // Le `static` est construit au premier passage, avec CETTE liaison,
        // et jamais reconstruit. Un second `begin()` ne rebâtirait donc pas
        // le cœur : il se contenterait de laisser croire qu'il l'a fait.
        // Autant le dire.
        if (_coeur) {
            IIOT_LOG("[InstantIoT] begin() a deja ete appele — le second est ignore");
            return _coeur->connected();
        }
        static InstantIoTCoreBase coeur(lien);
        _coeur = &coeur;
        return coeur.begin();
    }

    /** À appeler dans `loop()`, sans condition. */
    void loop() { if (_coeur) _coeur->loop(); }

    bool connected() { return _coeur && _coeur->connected(); }

    void setHeartbeat(uint32_t intervalMs) {
        if (_coeur) _coeur->setHeartbeat(intervalMs);
        else tropTot();
    }

    void setSignalRateLimit(uint16_t framesPerSecond) {
        if (_coeur) _coeur->setSignalRateLimit(framesPerSecond);
        else tropTot();
    }

    /**
     * Écrit une valeur sur un signal.
     *
     * Le gabarit ne fait que relayer : c'est le cœur qui porte les
     * surcharges, `double` et `unsigned long` compris, pour que
     * `write(I0, analogRead(A0) * 3.3 / 4095.0)` compile.
     *
     * @return faux si rien n'est parti — pas de liaison, ou le plafond de
     *         trames par seconde a mangé l'appel. Aucun des deux n'est une
     *         erreur du croquis, et aucun ne mérite un redémarrage.
     */
    template <class V>
    bool write(SignalRef sig, V valeur) {
        if (!_coeur) return tropTot();
        return _coeur->write(sig, valeur);
    }

    /**
     * La configuration de la carte.
     *
     * Elle appartient au cœur, qui n'existe qu'après `begin()`. Avant,
     * il n'y a rien à rendre et pas de référence à inventer : ce dépôt
     * vide encaisse les écritures pour que l'appel ait un sens même trop
     * tôt, et le journal dit qu'elles ne s'appliqueront pas.
     */
    DeviceConfig& config() {
        if (_coeur) return _coeur->config();
        tropTot();
        static DeviceConfig sansEffet;
        return sansEffet;
    }

private:
    InstantIoTCoreBase* _coeur = nullptr;

    /**
     * Le seul point où l'on répond « pas encore ».
     *
     * Une fois, et pas à chaque tour : appelé depuis `loop()`, un journal
     * bavard noie la ligne utile et ralentit assez pour changer le
     * symptôme observé.
     */
    bool _plainteFaite = false;
    bool tropTot() {
        if (!_plainteFaite) {
            _plainteFaite = true;
            IIOT_LOG("[InstantIoT] appel avant begin() — sans liaison, rien ne part");
        }
        return false;
    }
};

}  // namespace iiot

/**
 * L'objet unique.
 *
 * Il porte le nom que portait le namespace, devenu `iiot` pour le lui
 * laisser : en C++ un objet et un espace de noms ne cohabitent pas sous le
 * même nom, et c'est le croquis qui a besoin du beau nom.
 */
extern iiot::Facade InstantIoT;

/**
 * Les noms que le croquis écrit, à portée sans rien déclarer.
 *
 * Sans ces lignes, chaque croquis devait commencer par
 * `using namespace iiot;` — une formule à recopier sans la comprendre,
 * dans une bibliothèque dont le but est qu'il n'y en ait pas.
 *
 * Ce sont des `using` nommés et non un `using namespace` : seuls ces
 * noms-là sortent, et les internes — `Facade`, `InstantIoTCoreBase`,
 * `SignalRegistrar`, les transports — restent rangés. Un croquis qui en
 * veut un écrit `iiot::` et sait alors qu'il descend d'un étage.
 *
 * Ils sont inconditionnels : une liaison qu'une carte n'a pas existe
 * quand même, en coquille, pour que le message d'erreur soit le vrai.
 */
using iiot::AccessPoint;
using iiot::WiFiLink;
using iiot::BluetoothLink;
using iiot::BLELink;
using iiot::SerialLink;

using iiot::Cloud;
using iiot::MyServer;
using iiot::SecureDestination;
using iiot::PlainDestination;

/** Les touches d'une croix — `WHEN_PAD_PRESSED(t) { if (t == DPadButton::A) … }` */
using iiot::DPadButton;
