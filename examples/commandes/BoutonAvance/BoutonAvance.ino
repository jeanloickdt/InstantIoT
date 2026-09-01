/*************************************************************
 * InstantIoT — un bouton avance
 *
 * Le meme vocabulaire que le bouton simple. Ce qui change est
 * dans l'app : l'apparence, le libelle, la couleur. La carte,
 * elle, recoit exactement la meme chose.
 *
 * Dans l'app : un bouton avance sur I0.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

int appuis = 0;

IAdvancedButton(I0) {
    WHEN_PRESSED {
        appuis++;
        digitalWrite(LED_BUILTIN, HIGH);
        InstantIoT.write(I1, appuis);      // un compteur, a afficher
    }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW); }
    WHEN_LONG_PRESSED  { appuis = 0; InstantIoT.write(I1, appuis); }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_BoutonAvance", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
