/*************************************************************
 * InstantIoT — une manette
 *
 * L'app envoie une position — deux nombres entre -1 et 1 — et le
 * bloc les recoit deja decodes. Le retour au centre arrive comme
 * une position lui aussi : (0, 0).
 *
 * Dans l'app : une manette sur I0.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define MOTEUR_GAUCHE 5
#define MOTEUR_DROIT  18

IJoystick(I0, float x, float y) {
    // Melange classique : l'avance vient de y, la rotation de x.
    float gauche = y + x;
    float droit  = y - x;
    analogWrite(MOTEUR_GAUCHE, (int)(constrain(gauche, 0.0f, 1.0f) * 255));
    analogWrite(MOTEUR_DROIT,  (int)(constrain(droit,  0.0f, 1.0f) * 255));
};

void setup() {
    Serial.begin(115200);
    pinMode(MOTEUR_GAUCHE, OUTPUT);
    pinMode(MOTEUR_DROIT,  OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Manette", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
