/*************************************************************
 * InstantIoT — un thermostat, les deux sens
 *
 * La carte publie ce qu'elle mesure, et obeit a ce qu'on lui
 * ecrit. C'est l'exemple qui montre le mieux ce que la 2.0 a
 * change.
 *
 * L'interessant est la seconde moitie. Une consigne est un ETAT,
 * pas un geste : le serveur la garde et la renvoie a la connexion.
 * Debranchez la carte, rebranchez-la demain — le bloc `ISignal(I5,…)`
 * repart avec la consigne d'hier, sans que personne n'ouvre l'app
 * et sans rien avoir ecrit en EEPROM.
 *
 * Un `ISimpleButton`, lui, ne serait PAS rejoue : personne n'a
 * appuye. Un etat se rappelle, un geste ne se rejoue pas.
 *
 * Signaux a declarer dans l'app, sur cette carte :
 *   I0  float   mesure    « Temperature »
 *   I5  float   consigne  « Consigne »     de 5 a 30, rejeu actif
 *   I6  bool    consigne  « Pompe »
 *   I7  texte   consigne  « Mode »
 *
 * A remplacer : WIFI_SSID, WIFI_PASS, DEVICE_TOKEN.
 *
 * Cartes : ESP32, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>
using namespace iiot;

const char* WIFI_SSID    = "MonWiFi";
const char* WIFI_PASS    = "MonMotDePasse";
const char* DEVICE_TOKEN = "COLLEZ_LE_JETON_ICI";

#if defined(ESP32)
  #define BROCHE_CAPTEUR 34
#else
  #define BROCHE_CAPTEUR A0
#endif
#define BROCHE_POMPE 4

float    consigne = 19.0f;
uint32_t derniereMesure = 0;

ISignal(I5, float cible) {
    consigne = cible;
    Serial.print("Consigne -> "); Serial.println(consigne);
};

ISignal(I6, bool marche) {
    digitalWrite(BROCHE_POMPE, marche ? HIGH : LOW);
};

ISignal(I7, const char* mode) {
    Serial.print("Mode -> "); Serial.println(mode);
};

// Facultatif : tout ce qui s'ecrit sur cette carte passe ici, y
// compris les adresses qu'aucun bloc ne reclame. C'est comme ca
// qu'on s'apercoit d'une adresse oubliee.
void onSignalWritten(const SignalEvent& e) {
    Serial.print("signal I"); Serial.print(e.address); Serial.println(" ecrit");
}

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_POMPE, OUTPUT);
    InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS), Cloud(DEVICE_TOKEN));
}

void loop() {
    InstantIoT.loop();

    if (millis() - derniereMesure >= 2000) {
        derniereMesure = millis();
        float volts = analogRead(BROCHE_CAPTEUR) * 3.3f / 4095.0f;
        InstantIoT.write(I0, volts * 100.0f);
    }
}
