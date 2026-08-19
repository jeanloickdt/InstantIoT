/*
 * EVENT — un fait adressé par un octet.
 *
 * Ce que ce fichier prouve tient en une phrase : **la lib sait qu'à I5 vit un
 * bouton parce que le croquis le lui a dit**, pas parce que la trame le dit.
 * La trame n'apporte que l'adresse et le genre — dix octets.
 *
 * Et il prouve la cohabitation : `ISimpleButton("btn1")` continue de
 * fonctionner à côté de `ISimpleButton(I5)`. Les croquis existants ne cassent
 * pas, ce qui est la condition pour que la migration soit possible.
 */

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../../src/core/BinaryCodec.hpp"
#include "../../src/core/InstantIoTCore.hpp"
#include "../../src/utils/InstantIoTWhen.hpp"

using namespace InstantIoT;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

// ── Les trames d'or ───────────────────────────────────────────────────────
// Calculées depuis la description du protocole, pas depuis ce code.

static const uint8_t PRESS_AT_I5[] = {
    0xAA, 0x01, 0x05, 0x00, 0x00, 0x01, 0x05, 0x21, 0x01, 0x6A
};
/** Pour un interrupteur, « basculé » vaut 0x03 — 0x04 est SetValue. */
static const uint8_t TOGGLE_AT_I6[] = {
    0xAA, 0x01, 0x05, 0x00, 0x00, 0x01, 0x06, 0x21, 0x03, 0xD9
};
static const uint8_t MOVED_AT_I7[] = {
    0xAA, 0x01, 0x0D, 0x00, 0x00, 0x01, 0x07, 0x21, 0x01,
    0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x80, 0xBE, 0xD1
};

// ── Les blocs, tels qu'un croquis les écrit ───────────────────────────────

static int  pressesAtI5 = 0;
static int  releasesAtI5 = 0;
ISimpleButton(I5) {
    WHEN_PRESSED  { pressesAtI5++; }
    WHEN_RELEASED { releasesAtI5++; }
};

static int  togglesAtI6 = 0;
static bool lastSwitchState = false;
ISwitch(I6) {
    WHEN_TOGGLED(on) { togglesAtI6++; lastSwitchState = on; }
};

static int   movesAtI7 = 0;
static float lastX = 0, lastY = 0;
IJoystick(I7) {
    WHEN_MOVED(x, y) { movesAtI7++; lastX = x; lastY = y; }
};

// Un bloc à l'adresse 0 — l'adresse la plus banale, et celle qui piège :
// un nœud adressé par NOM porte `address = 0` par défaut.
static int pressesAtI0 = 0;
ISimpleButton(I0) {
    WHEN_PRESSED { pressesAtI0++; }
};

// L'ANCIEN modèle, sur le même type de widget — il doit continuer de vivre.
// Le croisement : un ISignal pose sur une adresse qui recoit des evenements.
static int i20Signal = 0;
ISignal(I20, bool on) { (void)on; i20Signal++; };

// Et l'inverse : un bloc de widget sur une adresse qui recoit des valeurs.
static int i21Widget = 0;
ISimpleButton(I21) {
    WHEN_PRESSED { i21Widget++; }
};

static int pressesOnBtn1 = 0;
ISimpleButton("btn1") {
    WHEN_PRESSED { pressesOnBtn1++; }
};

// ── Le vrai coeur ─────────────────────────────────────────────────────────

struct NullTransport : ITransport {
    bool   begin() override                           { return true; }
    void   poll() override                            {}
    bool   connected() override                       { return true; }
    int    available() override                       { return 0; }
    int    read(uint8_t*, size_t) override            { return 0; }
    size_t write(const uint8_t*, size_t len) override { return len; }
};
static NullTransport nullTransport;

struct TestCore : InstantIoTCoreBase {
    TestCore() : InstantIoTCoreBase(nullTransport) {}
    using InstantIoTCoreBase::processFrame;
};
static TestCore core;

int main() {
    BinaryCodec codec;

    section("La table adresse → type, remplie par les blocs eux-memes");
    {
        // Personne n'a appelé de fonction d'enregistrement : les constructeurs
        // des registrars ont tourné avant main().
        ok(typeAtAddress(5) == TYPE_SIMPLEBUTTON, "I5 est un bouton, parce que le croquis le dit");
        ok(typeAtAddress(6) == TYPE_SWITCH,       "I6 est un interrupteur");
        ok(typeAtAddress(7) == TYPE_JOYSTICK,     "I7 est un joystick");
        ok(typeAtAddress(9) == 0,                 "I9 n'est ecoute par personne");
    }

    section("Une trame de dix octets suffit");
    {
        ok(sizeof(PRESS_AT_I5) == 10,
           "contre 19 pour la meme chose adressee par un nom de quatre lettres");

        uint8_t a = 0, ev = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        ok(BinaryCodec::decodeEvent(PRESS_AT_I5, sizeof(PRESS_AT_I5), a, ev, pl, pn), "la trame se decode");
        ok(a == 5 && ev == CMD_PRESS, "l'adresse et le genre, et rien d'autre");
        ok(pn == 0, "un appui n'a pas de charge utile");
    }

    section("Du fil jusqu'au bloc");
    {
        pressesAtI5 = releasesAtI5 = 0;
        core.processFrame(PRESS_AT_I5, sizeof(PRESS_AT_I5));
        ok(pressesAtI5 == 1, "WHEN_PRESSED s'est execute");
        ok(releasesAtI5 == 0, "et pas WHEN_RELEASED");

        // Trois appuis restent trois appuis. C'est LA raison pour laquelle un
        // EVENT ne peut pas etre une valeur : ecrire 1 trois fois n'en fait
        // qu'une transition, et deux appuis disparaitraient.
        core.processFrame(PRESS_AT_I5, sizeof(PRESS_AT_I5));
        core.processFrame(PRESS_AT_I5, sizeof(PRESS_AT_I5));
        ok(pressesAtI5 == 3, "trois appuis identiques comptent trois — un signal en aurait perdu deux");
    }

    section("Chaque adresse est lue selon SON type declare");
    {
        togglesAtI6 = 0;
        core.processFrame(TOGGLE_AT_I6, sizeof(TOGGLE_AT_I6));
        ok(togglesAtI6 == 1, "I6 a ete lu comme un interrupteur");
        ok(pressesAtI5 == 3, "et I5 n'a pas bouge");

        movesAtI7 = 0;
        core.processFrame(MOVED_AT_I7, sizeof(MOVED_AT_I7));
        ok(movesAtI7 == 1, "I7 a ete lu comme un joystick");
        // Une position est UNE chose : x et y arrivent ensemble, la carte ne
        // traverse jamais une position intermediaire qui n'a pas existe.
        ok(lastX == 0.5f && lastY == -0.25f, "x et y sont arrives dans la meme trame");
    }

    section("L'ancien modele vit toujours");
    {
        pressesOnBtn1 = 0;
        uint8_t frame[64];
        size_t n = codec.encode(frame, sizeof(frame), "dev1", "btn1",
                                TYPE_SIMPLEBUTTON, CMD_PRESS);
        core.processFrame(frame, n);
        ok(pressesOnBtn1 == 1, "ISimpleButton(\"btn1\") recoit encore — les croquis existants tiennent");
        ok(pressesAtI5 == 3, "…et le bloc adresse par octet ne l'a pas intercepte");
    }

    section("Les deux familles ne se croisent jamais");
    {
        // Un evenement adresse a l'octet 5 ne doit pas atteindre un bloc dont
        // le NOM serait « 5 » — et reciproquement.
        pressesOnBtn1 = 0;
        int before = pressesAtI5;

        uint8_t frame[64];
        size_t n = codec.encode(frame, sizeof(frame), "dev1", "5",
                                TYPE_SIMPLEBUTTON, CMD_PRESS);
        core.processFrame(frame, n);

        ok(pressesAtI5 == before, "un bloc adresse par octet ignore une trame nommee « 5 »");
        ok(pressesOnBtn1 == 0, "et btn1 n'a rien recu non plus");
    }

    section("Un bloc NOMME n'est jamais atteint par une adresse");
    {
        // Le piege exact : un noeud adresse par NOM porte `address = 0`. Sans
        // la garde, un EVENT sur I0 — une adresse parfaitement ordinaire —
        // declencherait tous les blocs nommes du croquis.
        static const uint8_t PRESS_AT_I0[] = { 0xAA, 0x01, 0x05, 0x00, 0x00, 0x01, 0x00, 0x21, 0x01, 0xAA };

        pressesOnBtn1 = 0;
        pressesAtI0 = 0;
        core.processFrame(PRESS_AT_I0, sizeof(PRESS_AT_I0));

        ok(pressesAtI0 == 1, "le bloc de I0 recoit bien");
        ok(pressesOnBtn1 == 0,
           "btn1 ne doit pas repondre a l'adresse 0 sous pretexte que son champ address vaut 0");
    }

    section("Ce que personne n'ecoute");
    {
        uint8_t body[] = { 0x00, 0x01, 0x09, 0x21, 0x01 };
        uint8_t crc = 0;
        for (uint8_t b : body) {
            crc ^= b;
            for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
        uint8_t frame[16] = { 0xAA, 0x01, 0x05, 0x00,
                              body[0], body[1], body[2], body[3], body[4], crc };

        int p = pressesAtI5, t = togglesAtI6, m = movesAtI7;
        core.processFrame(frame, 10);   // I9 : aucun bloc
        ok(pressesAtI5 == p && togglesAtI6 == t && movesAtI7 == m,
           "une adresse sans bloc ne declenche rien, et ne plante pas");
    }

    section("Le croisement se dit, au lieu de se taire");
    {
        // Un EVENT sur une adresse ou seul un ISignal ecoute.
        uint8_t frame[64];
        size_t n = codec.encodeSignal(frame, sizeof(frame), 20, SIGNAL_TAG_BOOL, (const uint8_t*)"\x01", 1);
        // ^ c'est un SIGNAL : il doit atteindre le bloc ISignal normalement.
        i20Signal = 0;
        Serial.clear();
        core.processFrame(frame, n);
        ok(i20Signal == 1, "un signal atteint bien son bloc ISignal");

        // Maintenant un EVENT a la meme adresse : personne du bon genre.
        uint8_t body[] = { 0x00, 0x01, 20, 0x21, 0x01 };
        uint8_t crc = 0;
        for (uint8_t b : body) {
            crc ^= b;
            for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
        uint8_t ev[16] = { 0xAA, 0x01, 0x05, 0x00, body[0], body[1], body[2], body[3], body[4], crc };

        i20Signal = 0;
        Serial.clear();
        core.processFrame(ev, 10);
        ok(i20Signal == 0, "l'evenement n'atteint pas le bloc ISignal — il ne sait pas le lire");
        ok(Serial.saw("an ISignal listens here"),
           "mais la carte le DIT : sans ca, l'utilisateur voit un bloc mort et cherche dans son croquis");
    }
    {
        // Une VALEUR sur une adresse ou seul un bloc de widget ecoute.
        uint8_t frame[64];
        uint8_t one[1] = { 1 };
        size_t n = codec.encodeSignal(frame, sizeof(frame), 21, SIGNAL_TAG_BOOL, one, 1);

        i21Widget = 0;
        Serial.clear();
        core.processFrame(frame, n);

        ok(i21Widget == 0, "la valeur n'atteint pas le bloc de widget");
        ok(Serial.saw("a widget block listens here"),
           "et le pendant du diagnostic est la aussi");
    }

    section("Ce qui n'est pas un EVENT n'est pas reclame");
    {
        uint8_t a = 0, ev = 0; const uint8_t* pl = nullptr; size_t pn = 0;
        // Un SIGNAL a la meme forme, un autre TYPE.
        uint8_t sig[64];
        uint8_t v[4] = {0, 0, 0, 0};
        size_t n = codec.encodeSignal(sig, sizeof(sig), 5, SIGNAL_TAG_FLOAT, v, 4);
        ok(!BinaryCodec::decodeEvent(sig, n, a, ev, pl, pn),
           "un signal a l'adresse 5 n'est pas un appui a l'adresse 5");

        uint8_t corrupted[sizeof(PRESS_AT_I5)];
        memcpy(corrupted, PRESS_AT_I5, sizeof(corrupted));
        corrupted[8] ^= 0xFF;
        ok(!BinaryCodec::decodeEvent(corrupted, sizeof(corrupted), a, ev, pl, pn),
           "un genre corrompu est refuse — un appui fantasme actionne du materiel");
    }

    printf("\n%d checks, %d failure%s\n", checks, failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
