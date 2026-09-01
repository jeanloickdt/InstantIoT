/*************************************************************
 * InstantIoT — se connecter a SON serveur
 *
 * Le croquis le plus court qui parle a un serveur InstantIoT
 * que vous hebergez : la carte rejoint votre WiFi, atteint le
 * serveur, et la LED clignote quand la liaison tient.
 *
 * A remplacer avant de televerser :
 *   WIFI_SSID, WIFI_PASS   → votre box
 *   SERVER_HOST            → l'IP ou le nom de votre serveur
 *   DEVICE_TOKEN           → le jeton de la carte
 *
 * Cartes : ESP32, Arduino Uno R4 WiFi
 *************************************************************/

#include <InstantIoT.h>

const char* WIFI_SSID    = "MonWiFi";
const char* WIFI_PASS    = "MonMotDePasse";

const char* SERVER_HOST  = "192.168.1.42";
const char* DEVICE_TOKEN = "COLLEZ_LE_JETON_ICI";

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
    Serial.println(InstantIoT.connected() ? "serveur : joint" : "serveur : pas joint");
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // La liaison, puis la destination : la carte rejoint d'abord un
    // reseau, puis atteint un serveur. En clair, parce qu'un serveur
    // chez vous ne sort pas de chez vous. Devant un serveur en TLS :
    //   MyServer(SERVER_HOST, DEVICE_TOKEN).secure()
    if (InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                         MyServer(SERVER_HOST, DEVICE_TOKEN))) {
        Serial.print("Connecte. IP locale : ");
        Serial.println(WiFi.localIP());
    } else {
        // `loop()` continue d'essayer, avec un delai qui s'allonge.
        Serial.println("Pas encore connecte — la carte reessaie.");
    }

    timers.every(2000, battement);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
