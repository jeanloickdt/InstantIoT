/*************************************************************
 * InstantIoT — le niveau d'une citerne, au capteur a ultrasons
 *
 * Ce croquis remplace quatre exemples d'avant : jauge, niveau
 * horizontal, niveau vertical, graphe. Ils avaient tous le meme
 * code — le meme capteur, le meme `write` — et ne differaient que
 * par le dessin, qui n'appartient plus a la carte.
 *
 * La carte ecrit un nombre. Ce qu'on en fait se decide dans l'app,
 * et se change sans reflasher.
 *
 * Dans l'app, sur I0 : une jauge. Ou un graphe, ou une metrique,
 * ou un niveau — c'est le meme signal.
 *
 * Cablage : HC-SR04, TRIG sur 14, ECHO sur 12 (via un pont
 * diviseur si la carte est en 3,3 V).
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

#define BROCHE_TRIG 14
#define BROCHE_ECHO 12

#define HAUTEUR_CITERNE_CM 100.0f   // fond plein
#define ZONE_MORTE_CM        5.0f   // le capteur ne voit rien de plus pres

uint32_t derniereMesure = 0;

/** @return le niveau en pourcentage, ou -1 si l'echo n'est pas revenu. */
float niveauCiterne() {
    digitalWrite(BROCHE_TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(BROCHE_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(BROCHE_TRIG, LOW);

    long duree = pulseIn(BROCHE_ECHO, HIGH, 30000);
    if (duree == 0) return -1.0f;

    float distanceCm = duree * 0.034f / 2.0f;
    float niveau = ((HAUTEUR_CITERNE_CM - distanceCm) /
                    (HAUTEUR_CITERNE_CM - ZONE_MORTE_CM)) * 100.0f;
    return constrain(niveau, 0.0f, 100.0f);
}

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_TRIG, OUTPUT);
    pinMode(BROCHE_ECHO, INPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Citerne", "12345678"));
}

void loop() {
    InstantIoT.loop();

    // Une mesure par seconde : un ultrason n'a rien de plus a dire
    // plus souvent, et la citerne ne se vide pas si vite.
    if (millis() - derniereMesure >= 1000) {
        derniereMesure = millis();
        float niveau = niveauCiterne();
        // Une mesure ratee ne s'envoie pas : mieux vaut la derniere
        // valeur connue qu'un zero invente.
        if (niveau >= 0.0f) InstantIoT.write(I0, niveau);
    }
}
