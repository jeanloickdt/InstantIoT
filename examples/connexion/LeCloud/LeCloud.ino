/*************************************************************
 * InstantIoT — se connecter au cloud, chiffre
 *
 * La carte rejoint votre WiFi puis ouvre une session TLS vers le
 * cloud InstantIoT. Le jeton et les valeurs ne passent jamais
 * lisibles sur internet.
 *
 * A remplacer avant de televerser :
 *   WIFI_SSID, WIFI_PASS   → votre box
 *   DEVICE_TOKEN           → le jeton donne par le panneau cloud
 *
 * Cartes : ESP32, Arduino Uno R4 WiFi (le TLS y est fait par le
 * modem embarque).
 *************************************************************/

// Ça ne se connecte pas ? Decommentez cette ligne AVANT l'include et
// rouvrez le moniteur serie : la lib dit alors a quelle etape elle
// bute — WiFi, TLS, ou poignee de main. Sans elle, ses journaux sont
// compiles hors du binaire et il ne reste que les messages ci-dessous,
// qui ne savent rien.
// #define INSTANTIOT_DEBUG 1

#include <InstantIoT.h>

const char* WIFI_SSID    = "MonWiFi";
const char* WIFI_PASS    = "MonMotDePasse";
const char* DEVICE_TOKEN = "COLLEZ_LE_JETON_ICI";

// Le cloud InstantIoT, port 9443. Pour une preproduction ou un cloud
// que vous hebergez, nommez l'hote et le port : voir `.at(...)` plus bas.
const char* SERVER_HOST  = "instantiot.cloud";
const uint16_t SERVER_PORT = 9443;

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

InstantTimer timers;
bool     allumee = false;

// Le battement de la LED, une fois par seconde. `timers.every` plutot
// qu'un `millis() - dernier >= …` recopie a la main, et surtout pas un
// `delay()` : il arreterait aussi la lecture des trames qui arrivent.
void battement() {
    allumee = InstantIoT.connected() && !allumee;
    digitalWrite(LED_BUILTIN, allumee ? HIGH : LOW);
    Serial.println(InstantIoT.connected() ? "cloud : joint (TLS)" : "cloud : pas joint");
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // `Cloud(...)` chiffre et verifie l'identite du serveur contre les
    // racines Let's Encrypt embarquees. Trois facons d'en sortir, et
    // elles ne disent pas la meme chose :
    //
    //   .withCertificate(MA_RACINE)  votre autorite a vous — l'identite
    //                                reste verifiee, contre elle
    //   .withoutCertCheck()          chiffre, mais n'importe qui peut se
    //                                faire passer pour le serveur. Pour un
    //                                premier demarrage, pas pour la suite
    //   .plaintext()                 pas de chiffrement du tout, pour une
    //                                carte sans pile TLS. Le jeton passe
    //                                alors lisible, et c'est le prix
    Serial.print("cible : "); Serial.print(SERVER_HOST);
    Serial.print(":");         Serial.println(SERVER_PORT);

    if (InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                         Cloud(DEVICE_TOKEN).at(SERVER_HOST, SERVER_PORT))) {
        Serial.print("Connecte au cloud. IP locale : ");
        Serial.println(WiFi.localIP());
    } else {
        // Echouer ici est normal : au demarrage la box n'est pas toujours
        // prete. `loop()` continue d'essayer, avec un delai qui s'allonge.
        Serial.println("Pas encore connecte — la carte reessaie.");
    }

    timers.every(2000, battement);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
