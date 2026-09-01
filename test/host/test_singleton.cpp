/*
 * L'objet unique — ce qu'il promet, et les deux endroits où il ment.
 *
 * Ce que le croquis attend de `InstantIoT` tient en une phrase : **il
 * s'appelle depuis n'importe où**. Trois de ces « n'importe où » ne se
 * vérifient pas à la relecture :
 *
 *   - avant `begin()`, quand il n'y a pas encore de liaison ;
 *   - depuis un bloc `ISignal`, c'est-à-dire pendant que le cœur est en
 *     train de lire une trame ;
 *   - deux fois `begin()`, ce qu'un croquis remanié finit toujours par
 *     faire.
 */

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../../src/InstantIoT.h"
#include "../../src/utils/InstantIoTWhen.hpp"

using namespace iiot;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

// ── Une liaison de papier ─────────────────────────────────────────────────
//
// Elle garde ce qui sort et rend ce qu'on lui a mis dedans : de quoi voir
// une écriture partir, et une trame arriver.

struct LiaisonDePapier : ITransport {
    uint8_t sorti[256];  size_t sortiLen = 0;
    uint8_t aLire[256];  size_t aLireLen = 0, aLirePos = 0;
    bool    ouverte = false;

    bool begin() override      { ouverte = true; return true; }
    void poll() override       {}
    bool connected() override  { return ouverte; }
    int  available() override  { return (int)(aLireLen - aLirePos); }

    int read(uint8_t* buf, size_t len) override {
        size_t n = aLireLen - aLirePos;
        if (n > len) n = len;
        memcpy(buf, aLire + aLirePos, n);
        aLirePos += n;
        return (int)n;
    }

    size_t write(const uint8_t* buf, size_t len) override {
        if (sortiLen + len > sizeof(sorti)) return 0;
        memcpy(sorti + sortiLen, buf, len);
        sortiLen += len;
        return len;
    }

    void depose(const uint8_t* trame, size_t len) {
        memcpy(aLire, trame, len);
        aLireLen = len;
        aLirePos = 0;
    }
    void oublie() { sortiLen = 0; }
};

static LiaisonDePapier liaison;

// ── Le bloc qui répond ────────────────────────────────────────────────────
//
// Le cas dont on veut être sûr : écrire DEPUIS un bloc, donc pendant que le
// cœur est en train de traiter une trame reçue.

static int i5Recu = 0;
ISignal(I5, float consigne) {
    i5Recu++;
    InstantIoT.write(I6, consigne * 2.0f);
};

// ── Lecture d'une trame de signal ─────────────────────────────────────────

struct Lu { bool ok = false; uint8_t addr = 0, tag = 0; float valeur = 0; };

static Lu lire(const uint8_t* trame, size_t len) {
    Lu r;
    const uint8_t* charge = nullptr;
    size_t chargeLen = 0;
    bool rappel = false;
    r.ok = BinaryCodec::decodeSignal(trame, len, r.addr, r.tag, charge, chargeLen, rappel);
    if (r.ok && chargeLen == 4) memcpy(&r.valeur, charge, 4);
    return r;
}

int main() {
    section("Avant begin() : rien, et surtout pas un plantage");
    {
        // Aucune liaison n'a été ouverte. Un croquis qui écrit ici s'est
        // trompé — la lib doit le lui dire, pas le faire redémarrer.
        ok(!InstantIoT.connected(), "personne n'est connecte");
        ok(!InstantIoT.write(I0, 1.0f), "une ecriture ne part pas");
        ok(!InstantIoT.write(I0, 42), "…quel que soit le type");
        ok(!InstantIoT.write(I0, "bonjour"), "…texte compris");
        InstantIoT.loop();          // ne doit rien faire, et surtout revenir
        InstantIoT.setHeartbeat(5000);
        InstantIoT.setSignalRateLimit(30);
        ok(true, "loop() et les reglages passent sans liaison");
    }

    section("Apres begin() : l'ecriture atteint la liaison");
    {
        ok(InstantIoT.begin(liaison), "la liaison s'ouvre");
        ok(InstantIoT.connected(), "…et se declare connectee");

        liaison.oublie();
        ok(InstantIoT.write(I5, 23.4f), "une valeur part");
        Lu r = lire(liaison.sorti, liaison.sortiLen);
        ok(r.ok, "ce qui est sorti est bien une trame de signal");
        ok(r.addr == 5, "a la bonne adresse");
        ok(r.valeur > 23.39f && r.valeur < 23.41f, "avec la bonne valeur");
    }

    section("Deux fois begin() : la seconde ne refait pas de coeur");
    {
        LiaisonDePapier autre;
        InstantIoT.begin(autre);
        liaison.oublie();
        autre.oublie();
        InstantIoT.write(I5, 1.0f);
        ok(liaison.sortiLen > 0, "la premiere liaison sert toujours");
        ok(autre.sortiLen == 0, "la seconde n'a rien recu — begin() ne se rejoue pas");
    }

    section("Depuis un bloc : ecrire pendant qu'on lit");
    {
        // On fabrique la trame d'arrivee avec notre propre encodeur, puis on
        // la depose dans la liaison : le cœur la lira au prochain loop().
        uint8_t entrante[64];
        BinaryCodec codec;
        uint8_t charge[4];
        float consigne = 10.0f;
        memcpy(charge, &consigne, 4);
        size_t n = codec.encodeSignal(entrante, sizeof(entrante), 5, SIGNAL_TAG_FLOAT, charge, 4);
        ok(n > 0, "la trame d'arrivee s'encode");

        liaison.depose(entrante, n);
        liaison.oublie();
        i5Recu = 0;

        InstantIoT.loop();

        ok(i5Recu == 1, "le bloc ISignal a bien ete appele");
        Lu r = lire(liaison.sorti, liaison.sortiLen);
        ok(r.ok, "et l'ecriture faite DEPUIS le bloc est sortie");
        ok(r.addr == 6, "a l'adresse que le bloc a demandee");
        ok(r.valeur > 19.9f && r.valeur < 20.1f, "avec la valeur calculee dans le bloc");
    }

    printf(failures ? "\n%d ECHEC(S) sur %d\n" : "\ntout passe (%d/%d)\n",
           failures ? failures : checks, checks);
    return failures ? 1 : 0;
}
