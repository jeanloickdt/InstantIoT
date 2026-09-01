/*************************************************************
 * InstantIoT — un choix segmente
 *
 * L'app envoie l'indice du segment choisi, a partir de 0. La carte
 * ne connait pas les libelles : c'est l'app qui les porte, et c'est
 * tres bien — les changer ne demande pas de reflasher.
 *
 * Dans l'app : un choix segmente sur I0, trois segments
 * (« Eco », « Confort », « Boost »).
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define BROCHE_PWM 5

const int PUISSANCES[] = { 60, 160, 255 };

ISegmentedSwitch(I0, int choix) {
    if (choix < 0 || choix > 2) return;   // un indice hors table ne fait rien
    analogWrite(BROCHE_PWM, PUISSANCES[choix]);
    InstantIoT.write(I1, choix);
};

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_PWM, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Choix", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
