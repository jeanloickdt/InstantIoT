/*************************************************************
 * 📡 InstantIoT — Wi-Fi Provisioning Demo
 *
 * Démontre le flow de provisioning sans-cable :
 *   1. Au 1er flash USB, le device crée un Wi-Fi "InstantIoT-Setup-XXXX"
 *   2. L'user ouvre l'app InstantIoT → Maker Pro → 📡 Provision a device
 *   3. L'app envoie SSID/pass home Wi-Fi + serveur IP/port + token
 *   4. Le device commit en NVS + reboot
 *   5. Au reboot, lit la NVS → se connecte au routeur + au serveur
 *
 * Si l'user change de réseau (déménagement, nouveau routeur), il
 * fait long-press sur GPIO 0 (5s) → factory reset → retour au step 1.
 *
 * Hardware testé : ESP32 DevKit (LED builtin GPIO 2, bouton BOOT GPIO 0)
 *
 * Widgets exemple : 1 LED on/off via app, 1 gauge qui envoie une valeur
 * random toutes les secondes.
 *************************************************************/

#include <InstantIoTProvisionable.hpp>
#include <InstantIoTWiFiServer.hpp>
#include <utils/InstantIoTTimer.hpp>

// ════════════════════════════════════════════════════════════════
// 🔧 Globals
// ════════════════════════════════════════════════════════════════

InstantIoTProvisionable provisioner;
InstantIoTWiFiServer*   instant = nullptr;
InstantTimer            timers;

const uint8_t PIN_LED = 2;

// ════════════════════════════════════════════════════════════════
// 🎯 Widgets — actifs uniquement en mode RUNNING (post-provisioning)
// ════════════════════════════════════════════════════════════════

ISimpleButton("btn1") {
    WHEN_TOGGLED(isOn) {
        analogWrite(PIN_LED, isOn ? 255 : 0);
        Serial.print("btn1 LED ");
        Serial.println(isOn ? "ON" : "OFF");
    }
    WHEN_PRESSED        { }
    WHEN_RELEASED       { }
    WHEN_LONG_PRESSED   { }
};

void update_gauge1() {
    if (!instant || !instant->connected()) return;
    static float v = 50.0f;
    v += (random(-50, 51) / 10.0f);
    if (v < 0.0f)   v = 0.0f;
    if (v > 100.0f) v = 100.0f;
    instant->gauge("gauge1").update(v, 0.0f, 100.0f);
}

// ════════════════════════════════════════════════════════════════
// 🚀 SETUP
// ════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_LED, OUTPUT);
    analogWrite(PIN_LED, 0);

    // (Optionnel) Active le bouton hardware de factory reset.
    // Long-press 5s sur GPIO 0 → wipe NVS + reboot → mode AP.
    // L'user n'a pas besoin de re-flasher USB pour changer de réseau.
    provisioner.useFactoryResetButton(0);  // default hold = 5 secondes

    // Init NVS + LED + bouton (non-bloquant)
    provisioner.begin();

    Serial.println();
    Serial.println("[InstantIoT] Checking NVS for provisioned config...");

    // ⏳ Bloque ici :
    //   - Si NVS vide : démarre l'AP "InstantIoT-Setup-XXXX", attend
    //     l'app, COMMIT côté app → ESP.restart() (cette ligne ne return
    //     pas en mode cold).
    //   - Si NVS plein : retourne immédiatement la config.
    iiot::ProvisionedConfig cfg = provisioner.awaitConfig();

    Serial.println("[InstantIoT] Provisioning OK — connecting...");
    Serial.print("  SSID:        "); Serial.println(cfg.ssid);
    Serial.print("  Server:      "); Serial.print(cfg.serverIp);
    Serial.print(":"); Serial.println(cfg.serverPort);
    Serial.print("  Device tok:  "); Serial.println(cfg.deviceToken);

    // 🚀 Démarre le serveur normalement avec les creds NVS
    static InstantIoTWiFiServer s(cfg.serverIp, cfg.serverPort, cfg.deviceToken);
    instant = &s;
    instant->setHeartbeat(1000);
    if (!instant->begin(cfg.ssid, cfg.password)) {
        Serial.println("[InstantIoT] Server connect failed — will auto-retry.");
    }

    // Timer pour pousser le gauge
    timers.every(1000, update_gauge1);

    Serial.println("[InstantIoT] Setup done.");
}

// ════════════════════════════════════════════════════════════════
// 🔁 LOOP
// ════════════════════════════════════════════════════════════════

void loop() {
    provisioner.loop();   // gère LED + bouton reset hardware
    if (instant) instant->loop();
    timers.run();
}
