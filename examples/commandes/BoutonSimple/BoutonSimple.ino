/*************************************************************
 * InstantIoT — un bouton simple
 *
 * Trois gestes, trois blocs, pas un seul `if` a ecrire.
 *
 * Dans l'app : posez un bouton simple sur le signal I0 de cette
 * carte. Le geste voyage comme une valeur — 1 appui, 0 relachement,
 * 2 appui long — et le bloc la retraduit en geste.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

ISimpleButton(I0) {
    WHEN_PRESSED       { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED      { digitalWrite(LED_BUILTIN, LOW);  }
    WHEN_LONG_PRESSED  {
        // Repondre depuis le bloc est le cas normal : la carte
        // previent l'app que l'appui long a bien ete compris.
        InstantIoT.write(I1, "appui long");
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Bouton", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
