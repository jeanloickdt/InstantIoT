// Le battement de coeur ne porte pas d'identite de carte.
//
// C'est le relais qui sait quelle carte parle : il l'a appris du jeton, a la
// poignee de main. Toutes les trames SIGNAL le disent deja en ecrivant
// DEV_COUNT = 0 — `encodeSignal`, « the relay knows the board ». Le battement
// passait par l'encodeur historique et emportait, lui, un identifiant que
// personne ne lit et qui etait le meme sur toutes les cartes sans numero de
// serie.
//
// Ce test tient les deux bouts : rien par defaut, et ce que le croquis a
// nomme s'il en a nomme un.
#include "core/BinaryCodec.hpp"
#include "core/InstantIoTDeviceConfig.hpp"
using namespace iiot;

static int failures = 0;
#define CHECK(cond, quoi) do { if(!(cond)) { printf("ECHEC: %s\n", quoi); failures++; } } while(0)

/** DEV_COUNT est le premier octet du corps, donc le 5e de la trame. */
static uint8_t devCountOf(const uint8_t* frame) { return frame[4]; }

int main() {
    BinaryCodec codec;
    uint8_t frame[128];

    // ── par defaut : aucune carte nommee, aucune carte envoyee ──
    {
        DeviceConfig config;
        size_t n = codec.encode(frame, sizeof(frame),
                                config.getDeviceId(), "", TYPE_HEARTBEAT, 0);
        CHECK(n > 0, "le battement s'encode");
        CHECK(devCountOf(frame) == 0, "DEV_COUNT = 0 par defaut");
        CHECK(config.getDeviceId()[0] == '\0', "aucun identifiant invente");
    }

    // ── le croquis en nomme un : il part, comme avant ──
    {
        DeviceConfig config;
        config.setDeviceId("serre-nord");
        size_t n = codec.encode(frame, sizeof(frame),
                                config.getDeviceId(), "", TYPE_HEARTBEAT, 0);
        CHECK(n > 0, "le battement s'encode");
        CHECK(devCountOf(frame) == 1, "DEV_COUNT = 1 quand le croquis nomme");
        CHECK(frame[5] == 10, "DEV_LEN = 10");
        CHECK(memcmp(frame + 6, "serre-nord", 10) == 0, "le nom du croquis");
    }

    // ── la trame est plus courte de ce qu'elle n'invente plus ──
    {
        DeviceConfig muet;
        size_t sansNom = codec.encode(frame, sizeof(frame),
                                      muet.getDeviceId(), "", TYPE_HEARTBEAT, 0);
        DeviceConfig nomme;
        nomme.setDeviceId("device_DEADBEEF");   // ce qui partait avant
        size_t avecNom = codec.encode(frame, sizeof(frame),
                                      nomme.getDeviceId(), "", TYPE_HEARTBEAT, 0);
        CHECK(avecNom - sansNom == 16, "16 octets par battement, toutes les 5 s");
    }

    printf(failures ? "%d FAILURE(S)\n" : "all pass\n", failures);
    return failures;
}
