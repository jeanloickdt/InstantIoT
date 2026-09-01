/*************************************************************
 * InstantIoT — diagnostiquer une liaison, sans telephone
 *
 * Quand rien ne marche, la question est toujours la meme : ou est-ce
 * que ca s'arrete ? Ce croquis y repond avec la LED de la carte et
 * le moniteur serie, avant meme qu'un telephone soit ouvert.
 *
 *   LED eteinte           le WiFi n'est pas joint
 *   LED allumee fixe      le WiFi est joint, le serveur non
 *   LED qui clignote      le serveur est joint — tout va bien
 *
 * A remplacer : WIFI_SSID, WIFI_PASS, SERVER_HOST, DEVICE_TOKEN.
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

void faireLePoint() {
    // `WiFi` est l'objet du cœur Arduino : la question « suis-je sur le
    // reseau ? » ne regarde pas InstantIoT, et la facade ne la reexpose
    // pas. `InstantIoT.connected()`, elle, ne repond que de la liaison
    // jusqu'au serveur.
    bool surLeReseau  = (WiFi.status() == WL_CONNECTED);
    bool surLeServeur = InstantIoT.connected();

    if (surLeServeur) {
        allumee = !allumee;
        digitalWrite(LED_BUILTIN, allumee ? HIGH : LOW);
        Serial.println("serveur joint");
        InstantIoT.write(I0, (long)(millis() / 1000));   // secondes depuis le demarrage
    } else if (surLeReseau) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("WiFi joint, serveur non — verifiez l'adresse, le port, le jeton");
    } else {
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("WiFi non joint — verifiez le nom du reseau et le mot de passe");
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println();
    Serial.println("=== InstantIoT — diagnostic ===");
    Serial.print("serveur : "); Serial.println(SERVER_HOST);

    InstantIoT.begin(WiFiLink(WIFI_SSID, WIFI_PASS),
                     MyServer(SERVER_HOST, DEVICE_TOKEN));

    timers.every(1000, faireLePoint);
}

void loop() {
    InstantIoT.loop();
    timers.run();
}
