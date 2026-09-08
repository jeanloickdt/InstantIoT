/*************************************************************
 * InstantIoT — le cloud par un câble, et chiffré
 *
 * Un ESP32, un module W5500, et le jeton ne voyage plus en clair. C'est le
 * seul montage Ethernet de cette bibliothèque qui atteint le cloud
 * chiffré, et il vaut la peine de savoir pourquoi.
 *
 * ## La même puce, deux façons de s'en servir
 *
 * La bibliothèque `Ethernet` d'Arduino utilise la pile TCP câblée DANS le
 * W5500. C'est ce qui la rend utilisable sur un Mega — et c'est aussi ce
 * qui interdit le chiffrement : un client TLS ne peut pas envelopper une
 * socket qui vit dans un autre composant.
 *
 * Le cœur ESP32 sait faire autrement. Il pilote le W5500 comme une carte
 * réseau, derrière lwIP, et à partir de là le TLS de la plateforme
 * fonctionne sans savoir qu'il y a un câble.
 *
 * Ce croquis prend ce chemin-là. Sur toute autre carte il ne compile pas,
 * et le message dit laquelle des deux raisons s'applique.
 *
 * Cartes : ESP32 uniquement (esp32, S2, S3, C3, C6).
 *
 *************************************************************
 * ⚠️ LES TROIS BROCHES SONT LES VÔTRES
 *************************************************************
 *
 * Un module W5500 n'a aucun format imposé — on le câble. Les valeurs
 * ci-dessous sont le montage le plus répandu, pas une norme. Vérifiez-les
 * contre votre carte avant de téléverser : une mauvaise broche CS donne
 * « Driver refused to start » au démarrage.
 *
 * Si vos broches SCK / MISO / MOSI ne sont pas celles par défaut, appelez
 * `SPI.begin(sck, miso, mosi)` avant `InstantIoT.begin(...)`.
 *************************************************************/

// L'Ethernet est en OPT-IN : la ligne doit précéder l'include. Le système de
// compilation d'Arduino découvre les bibliothèques en LISANT les directives
// `#include`, donc un include conditionnel n'est jamais vu.
#define INSTANTIOT_ETHERNET 1

#include <InstantIoT.h>

const char* DEVICE_TOKEN = "PASTE_TOKEN_HERE";

// Le câblage du module. À vérifier contre le vôtre.
#define W5500_CS   5
#define W5500_IRQ  4
#define W5500_RST 14

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;

// Identique aux croquis WiFi, et c'est le sujet : un signal ne sait pas par
// où il a voyagé.
ISimpleButton(I0) {
    WHEN_PRESSED  { digitalWrite(LED_BUILTIN, HIGH); }
    WHEN_RELEASED { digitalWrite(LED_BUILTIN, LOW);  }
};

void envoyerLaMesure() {
    InstantIoT.write(I1, (float)(analogRead(A0) * 3.3f / 4095.0f));
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // `Cloud(...)` chiffre et vérifie l'identité du serveur contre les
    // racines Let's Encrypt embarquées. Les mêmes sorties que sur le WiFi :
    //
    //   .withCertificate(MA_RACINE)  votre autorité, l'identité reste vérifiée
    //   .withoutCertCheck()          chiffré, mais n'importe qui peut se faire
    //                                passer pour le serveur
    //   .plaintext()                 rien du tout, et le jeton devient lisible
    if (InstantIoT.begin(EthernetLink(W5500_CS, W5500_IRQ, W5500_RST),
                         Cloud(DEVICE_TOKEN))) {
        Serial.print("Joint, chiffre. IP locale : ");
        Serial.println(ETH.localIP());
    } else {
        Serial.println("Pas encore joint — la carte continue d'essayer.");
    }

    timers.every(2000, envoyerLaMesure);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
