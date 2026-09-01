/*************************************************************
 * InstantIoT — l'arret d'urgence
 *
 * Deux etats, et un seul compte : declenche, ou rearme. Tant que
 * l'arret est actif, la carte refuse de bouger — c'est le croquis
 * qui tient cette regle, pas l'app.
 *
 * Dans l'app : un bouton d'urgence sur I0.
 *
 * Cartes : ESP32, ESP8266, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

#define BROCHE_MOTEUR 5

bool arretActif = false;

IEmergencyButton(I0) {
    WHEN_TRIGGERED {
        arretActif = true;
        analogWrite(BROCHE_MOTEUR, 0);
        InstantIoT.write(I1, "ARRET");
    }
    WHEN_RESET {
        arretActif = false;
        InstantIoT.write(I1, "pret");
    }
};

// Une consigne de vitesse, ignoree tant que l'arret est actif.
ISignal(I2, float vitesse) {
    if (arretActif) return;
    analogWrite(BROCHE_MOTEUR, (int)(vitesse * 255.0f / 100.0f));
};

void setup() {
    Serial.begin(115200);
    pinMode(BROCHE_MOTEUR, OUTPUT);
    InstantIoT.begin(AccessPoint("InstantIoT_Urgence", "12345678"));
}

void loop() {
    InstantIoT.loop();
}
