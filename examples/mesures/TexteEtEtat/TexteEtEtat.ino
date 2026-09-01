/*************************************************************
 * InstantIoT — du texte et un etat
 *
 * Tout ne se mesure pas. Une carte a aussi des choses a DIRE — le
 * mode dans lequel elle est, ce qu'elle vient de faire — et des
 * etats a montrer.
 *
 * Un signal porte au plus 48 caracteres. C'est une valeur, pas un
 * journal : ce qui ne tient pas ici n'avait rien a y faire.
 *
 * Dans l'app, sur I0 : un afficheur de texte. Sur I1 : une LED.
 * Sur I2 : une metrique.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

const char* MODES[] = { "Eco", "Confort", "Boost" };
int   modeCourant = 0;
long  cyclesFaits = 0;
uint32_t dernierePublication = 0;

// L'app choisit le mode ; la carte le renvoie en toutes lettres,
// pour que l'afficheur montre ce que la carte a COMPRIS et non ce
// que le telephone a envoye.
ISegmentedSwitch(I3, int choix) {
    if (choix < 0 || choix > 2) return;
    modeCourant = choix;
    InstantIoT.write(I0, MODES[modeCourant]);
};

void setup() {
    Serial.begin(115200);
    InstantIoT.begin(AccessPoint("InstantIoT_Texte", "12345678"));
}

void loop() {
    InstantIoT.loop();

    if (millis() - dernierePublication >= 3000) {
        dernierePublication = millis();
        cyclesFaits++;
        InstantIoT.write(I0, MODES[modeCourant]);
        InstantIoT.write(I1, modeCourant == 2);     // « Boost » allume la LED
        InstantIoT.write(I2, cyclesFaits);
    }
}
