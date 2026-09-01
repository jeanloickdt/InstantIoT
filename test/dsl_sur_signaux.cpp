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
static bool bas = false, gauche = false, droite = false, centre = false;
static bool relacheQuelconque = false;
static int  toucheVue = -1, toucheLongue = -1, toucheRelachee = -1;
IDirectionPad(I3) {
    WHEN_UP           { haut = true; }
    WHEN_DOWN         { bas = true; }
    WHEN_LEFT         { gauche = true; }
    WHEN_RIGHT        { droite = true; }
    WHEN_CENTER       { centre = true; }
    WHEN_UP_LONG      { hautLong = true; }
    WHEN_DOWN_RELEASE { basRelache = true; }
    WHEN_RELEASED_ANY { relacheQuelconque = true; }
    WHEN_PAD_PRESSED(t)      { toucheVue = (int)t; }
    WHEN_PAD_LONG_PRESSED(t) { toucheLongue = (int)t; }
    WHEN_PAD_RELEASED(t)     { toucheRelachee = (int)t; }
}

static bool appui2 = false, relache2 = false, long2 = false;
IAdvancedButton(I4) {
    WHEN_PRESSED      { appui2 = true; }
    WHEN_RELEASED     { relache2 = true; }
    WHEN_LONG_PRESSED { long2 = true; }
}

static bool arret = false, rearme = false;
IEmergencyButton(I6) {
    WHEN_TRIGGERED { arret = true; }
    WHEN_RESET     { rearme = true; }
}

static float curseurH = -1, curseurV = -1;
IHorizontalSlider(I7, float v) { curseurH = v; }
IVerticalSlider(I8, float v)   { curseurV = v; }

static int segment = -1;
ISegmentedSwitch(I10, int index) { segment = index; }

static bool lampe = false;
ISwitch(I1, bool on) { lampe = on; }

static float consigne = 0;
ISignal(I5, float v) { consigne = v; }

// ── Le banc ───────────────────────────────────────────────────
static void envoie(uint8_t addr, float v) {
    iiot::SignalEvent e; e.address = addr;
    e.value.tag = iiot::SIGNAL_TAG_FLOAT;
    e.value.number = v; e.value.integer = (long)v; e.value.flag = (v != 0.0f);
    iiot::dispatchSignal(e);
}
static void envoieTexte(uint8_t addr, const char* t) {
    iiot::SignalEvent e; e.address = addr;
    e.value.tag = iiot::SIGNAL_TAG_STRING; e.value.string = t;
    iiot::dispatchSignal(e);
}

int main() {
    envoieTexte(2, "0.42,-0.15");
    VERIFIE(jx > 0.41f && jx < 0.43f, "la manette recoit x");
    VERIFIE(jy < -0.14f && jy > -0.16f, "la manette recoit y");

    envoie(0, 1.0f); VERIFIE(appui, "1 = appui");
    envoie(0, 0.0f); VERIFIE(relache, "0 = relachement");
    envoie(0, 2.0f); VERIFIE(longAppui, "2 = appui long");

    envoieTexte(3, "UP");           VERIFIE(haut, "UP");
    envoieTexte(3, "DOWN");         VERIFIE(bas, "DOWN");
    envoieTexte(3, "LEFT");         VERIFIE(gauche, "LEFT");
    envoieTexte(3, "RIGHT");        VERIFIE(droite, "RIGHT");
    envoieTexte(3, "CENTER");       VERIFIE(centre, "CENTER");
    envoieTexte(3, "UP_LONG");      VERIFIE(hautLong, "UP_LONG");
    envoieTexte(3, "DOWN_RELEASE"); VERIFIE(basRelache, "DOWN_RELEASE");
    VERIFIE(relacheQuelconque, "WHEN_RELEASED_ANY prend n'importe quel relachement");
    envoieTexte(3, "A");            VERIFIE(toucheVue == (int)iiot::DPadButton::A,
                                            "WHEN_PAD_PRESSED rend la touche");
    envoieTexte(3, "B_LONG");       VERIFIE(toucheLongue == (int)iiot::DPadButton::B,
                                            "WHEN_PAD_LONG_PRESSED rend la touche");
    envoieTexte(3, "LEFT_RELEASE"); VERIFIE(toucheRelachee == (int)iiot::DPadButton::Left,
                                            "WHEN_PAD_RELEASED rend la touche");

    envoie(4, 1.0f); VERIFIE(appui2,   "IAdvancedButton : appui");
    envoie(4, 0.0f); VERIFIE(relache2, "IAdvancedButton : relachement");
    envoie(4, 2.0f); VERIFIE(long2,    "IAdvancedButton : appui long");

    envoie(6, 1.0f); VERIFIE(arret,  "WHEN_TRIGGERED");
    envoie(6, 0.0f); VERIFIE(rearme, "WHEN_RESET");

    envoie(7, 42.0f); VERIFIE(curseurH > 41.9f && curseurH < 42.1f, "curseur horizontal");
    envoie(8, 17.0f); VERIFIE(curseurV > 16.9f && curseurV < 17.1f, "curseur vertical");

    envoie(10, 2.0f); VERIFIE(segment == 2, "choix segmente");

    envoie(1, 1.0f); VERIFIE(lampe, "l'interrupteur s'allume");
    envoie(1, 0.0f); VERIFIE(!lampe, "et s'eteint");

    envoie(5, 21.5f); VERIFIE(consigne > 21.4f && consigne < 21.6f, "ISignal recoit sa consigne");

    // Une adresse sans bloc ne casse rien.
    envoie(20, 1.0f);

    // ── Un RAPPEL ne reveille pas un geste ──
    appui = false; consigne = 0;
    iiot::SignalEvent r; r.address = 0;
    r.value.tag = iiot::SIGNAL_TAG_FLOAT; r.value.number = 1.0f;
    iiot::dispatchSignal(r, /* restore */ true);
    VERIFIE(!appui, "un rappel ne declenche pas ISimpleButton");

    iiot::SignalEvent r5; r5.address = 5;
    r5.value.tag = iiot::SIGNAL_TAG_FLOAT; r5.value.number = 19.0f;
    iiot::dispatchSignal(r5, /* restore */ true);
    VERIFIE(consigne > 18.9f && consigne < 19.1f, "mais il rend sa consigne a ISignal");

    printf(echecs ? "%d ECHEC(S)\n" : "tout passe\n", echecs);
    return echecs;
}
