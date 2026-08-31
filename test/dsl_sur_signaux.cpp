#include "core/SignalEvents.hpp"
#include "utils/InstantIoTWhen.hpp"
#include <cstdio>
static int echecs = 0;
#define VERIFIE(c, q) do { if(!(c)) { printf("ECHEC: %s\n", q); echecs++; } } while(0)

// ── Ce qu'un croquis ecrit desormais ──────────────────────────
static float jx = 9, jy = 9;
IJoystick(I2, float x, float y) { jx = x; jy = y; }

static bool appui = false, relache = false, longAppui = false;
ISimpleButton(I0) {
    WHEN_PRESSED      { appui = true; }
    WHEN_RELEASED     { relache = true; }
    WHEN_LONG_PRESSED { longAppui = true; }
}

static bool haut = false, hautLong = false, basRelache = false;
IDirectionPad(I3) {
    WHEN_UP           { haut = true; }
    WHEN_UP_LONG      { hautLong = true; }
    WHEN_DOWN_RELEASE { basRelache = true; }
}

static bool lampe = false;
ISwitch(I1, bool on) { lampe = on; }

static float consigne = 0;
ISignal(I5, float v) { consigne = v; }

// ── Le banc ───────────────────────────────────────────────────
static void envoie(uint8_t addr, float v) {
    InstantIoT::SignalEvent e; e.address = addr;
    e.value.tag = InstantIoT::SIGNAL_TAG_FLOAT;
    e.value.number = v; e.value.integer = (long)v; e.value.flag = (v != 0.0f);
    InstantIoT::dispatchSignal(e);
}
static void envoieTexte(uint8_t addr, const char* t) {
    InstantIoT::SignalEvent e; e.address = addr;
    e.value.tag = InstantIoT::SIGNAL_TAG_STRING; e.value.string = t;
    InstantIoT::dispatchSignal(e);
}

int main() {
    envoieTexte(2, "0.42,-0.15");
    VERIFIE(jx > 0.41f && jx < 0.43f, "la manette recoit x");
    VERIFIE(jy < -0.14f && jy > -0.16f, "la manette recoit y");

    envoie(0, 1.0f); VERIFIE(appui, "1 = appui");
    envoie(0, 0.0f); VERIFIE(relache, "0 = relachement");
    envoie(0, 2.0f); VERIFIE(longAppui, "2 = appui long");

    envoieTexte(3, "UP");           VERIFIE(haut, "UP");
    envoieTexte(3, "UP_LONG");      VERIFIE(hautLong, "UP_LONG");
    envoieTexte(3, "DOWN_RELEASE"); VERIFIE(basRelache, "DOWN_RELEASE");

    envoie(1, 1.0f); VERIFIE(lampe, "l'interrupteur s'allume");
    envoie(1, 0.0f); VERIFIE(!lampe, "et s'eteint");

    envoie(5, 21.5f); VERIFIE(consigne > 21.4f && consigne < 21.6f, "ISignal recoit sa consigne");

    // Une adresse sans bloc ne casse rien.
    envoie(9, 1.0f);

    // ── Un RAPPEL ne reveille pas un geste ──
    appui = false; consigne = 0;
    InstantIoT::SignalEvent r; r.address = 0;
    r.value.tag = InstantIoT::SIGNAL_TAG_FLOAT; r.value.number = 1.0f;
    InstantIoT::dispatchSignal(r, /* restore */ true);
    VERIFIE(!appui, "un rappel ne declenche pas ISimpleButton");

    InstantIoT::SignalEvent r5; r5.address = 5;
    r5.value.tag = InstantIoT::SIGNAL_TAG_FLOAT; r5.value.number = 19.0f;
    InstantIoT::dispatchSignal(r5, /* restore */ true);
    VERIFIE(consigne > 18.9f && consigne < 19.1f, "mais il rend sa consigne a ISignal");

    printf(echecs ? "%d ECHEC(S)\n" : "tout passe\n", echecs);
    return echecs;
}
