/*************************************************************
 * InstantIoT — un interrupteur
 *
 * Pas de `WHEN_` ici, et ce n'est pas un oubli : un interrupteur
 * n'a qu'une chose qui puisse lui arriver — sa valeur change. La
 * tete du bloc porte donc la donnee, et il n'y a rien a trier.
 *
 * Le type que vous declarez fait la conversion : ecrivez
 * `bool on`, et la valeur arrive en booleen.
 *
 * Dans l'app : un interrupteur sur I0.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define BROCHE_RELAIS 4

ISwitch(I0, bool allume) {
    digitalWrite(BROCHE_RELAIS, allume ? HIGH : LOW);
    InstantIoT.write(I1, allume ? "allume" : "eteint");
};

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_RELAIS, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Interrupteur", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
