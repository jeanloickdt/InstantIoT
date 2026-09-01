/*************************************************************
 * InstantIoT — un curseur vertical
 *
 * Le curseur ecrit une valeur, la carte la recoit. Rien a trier :
 * la tete du bloc porte la donnee.
 *
 * Attention au flot : un doigt qui glisse produit beaucoup de
 * valeurs. La lib en limite le debit, mais le croquis ne doit
 * rien faire de lent dans ce bloc.
 *
 * Dans l'app : un curseur vertical sur I0, borne de 0 a 100.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define BROCHE_PWM 5

IVerticalSlider(I0, float valeur) {
    analogWrite(BROCHE_PWM, (int)(valeur * 255.0f / 100.0f));
};

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_PWM, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_CurseurV", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
