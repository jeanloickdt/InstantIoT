/*************************************************************
 * InstantIoT — un petit tableau de bord
 *
 * Six signaux, les deux sens, et rien de plus qu'un croquis
 * ordinaire : un curseur qui regle une intensite, un bouton qui
 * allume, un interrupteur qui commute, et trois valeurs publiees.
 *
 * Ce que l'exemple montre vraiment : la carte ne sait pas ce que
 * l'app dessine. Elle recoit des valeurs a des adresses, et en
 * ecrit d'autres. Le tableau de bord se reorganise sans reflasher.
 *
 * Signaux a declarer dans l'app :
 *   recus   I0 curseur   I1 bouton   I2 interrupteur
 *   ecrits  I5 intensite  I6 duree de marche  I7 etat en toutes lettres
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
#define BROCHE_PWM   5
#define BROCHE_RELAIS 4

int      intensite = 0;
bool     relaisFerme = false;
uint32_t allumeDepuis = 0;
InstantTimer timers;

IHorizontalSlider(I0, float valeur) {
    intensite = (int)(valeur * 255.0f / 100.0f);
    analogWrite(BROCHE_PWM, intensite);
};

ISimpleButton(I1) {
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); allumeDepuis = millis(); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW);  }
};

ISwitch(I2, bool ferme) {
    relaisFerme = ferme;
    digitalWrite(BROCHE_RELAIS, ferme ? HIGH : LOW);
};

void publier() {
    InstantIoT.write(I5, intensite * 100 / 255);
    InstantIoT.write(I6, allumeDepuis ? (millis() - allumeDepuis) / 1000 : 0);
    InstantIoT.write(I7, relaisFerme ? "relais ferme" : "relais ouvert");
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BROCHE_PWM, OUTPUT);
    pinMode(BROCHE_RELAIS, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_TableauDeBord", "12345678"));
    timers.every(1000, publier);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
