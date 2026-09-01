/*************************************************************
 * InstantIoT — une temperature, au capteur analogique
 *
 * Un LM35 ou un TMP36 sur une entree analogique : trois fils, pas
 * de bibliotheque a installer. C'est la raison de ce choix — les
 * exemples d'avant demandaient la bibliotheque DHT, qu'il fallait
 * aller chercher avant meme de pouvoir compiler.
 *
 * Dans l'app, sur I0 : une metrique, ou un graphe, ou une jauge.
 * Sur I1 : une LED — elle s'allume quand il fait trop chaud.
 *
 * Cablage : sortie du capteur sur A0 (broche 34 sur ESP32).
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

#if defined(ESP32)
  #define BROCHE_CAPTEUR 34
#else
  #define BROCHE_CAPTEUR A0
#endif

float seuilAlerte = 30.0f;   // modifiable depuis l'app
uint32_t derniereMesure = 0;

/** LM35 : 10 mV par degre. TMP36 : ajoutez le decalage de 0,5 V. */
float temperature() {
    float volts = analogRead(BROCHE_CAPTEUR) * 3.3f / 4095.0f;
    return volts * 100.0f;
}

// Le seuil est un ETAT, pas un geste : au redemarrage, le serveur le
// renvoie et ce bloc le retrouve, sans que personne n'ouvre l'app.
ISignal(I2, float seuil) {
    seuilAlerte = seuil;
};

void setup() {
    Serial.begin(115200);
    InstantIoT.begin(AccessPoint("InstantIoT_Temperature", "12345678"));
}

void loop() {
    InstantIoT.loop();

    if (millis() - derniereMesure >= 2000) {
        derniereMesure = millis();
        float t = temperature();
        InstantIoT.write(I0, t);
        InstantIoT.write(I1, t > seuilAlerte);
    }
}
