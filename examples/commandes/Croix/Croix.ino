/*************************************************************
 * InstantIoT — la croix directionnelle
 *
 * Ici les `WHEN_` gagnent leur place : les touches font des choses
 * differentes, et les nommer evite l'escalier de `if`.
 *
 * Deux facons de lire, et elles ne servent pas au meme :
 *
 *   WHEN_UP { … }              une touche precise, un geste precis
 *   WHEN_PAD_PRESSED(touche)   toutes les touches, meme traitement
 *
 * Les cinq de la croix sont nommees. Les huit autres — A, B, X, Y
 * et les quatre formes — passent par la forme a variable.
 *
 * Dans l'app : une croix directionnelle sur I0.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

#define MOTEUR_GAUCHE 5
#define MOTEUR_DROIT  18

static void roule(int gauche, int droit) {
    analogWrite(MOTEUR_GAUCHE, gauche);
    analogWrite(MOTEUR_DROIT,  droit);
}

IDirectionPad(I0) {
    WHEN_UP      { roule(200, 200); }
    WHEN_DOWN    { roule(100, 100); }
    WHEN_LEFT    { roule(0,   200); }
    WHEN_RIGHT   { roule(200, 0);   }

    WHEN_UP_LONG { roule(255, 255); }   // plein gaz tant qu'on tient

    // N'importe quelle touche relachee arrete tout : savoir laquelle
    // n'interesse presque jamais.
    WHEN_RELEASED_ANY { roule(0, 0); }

    // Les touches d'action, traitees en bloc.
    WHEN_PAD_PRESSED(touche) {
        if (touche == DPadButton::A) InstantIoT.write(I1, "klaxon");
        if (touche == DPadButton::B) InstantIoT.write(I1, "phares");
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(MOTEUR_GAUCHE, OUTPUT);
    pinMode(MOTEUR_DROIT,  OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Croix", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
