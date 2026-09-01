#pragma once
/**
 * ============================================================
 * 🔐 TlsClient_ESP32.hpp — TLS transport to InstantIoT Cloud
 * ============================================================
 *
 * Variante **TLS** de TcpClient_ESP32 : l'ESP32 se connecte en
 * WiFi Station puis ouvre une connexion **chiffrée (TLS)** vers un
 * serveur InstantIoT Cloud. Le token et toutes les frames voyagent
 * chiffrés — jamais en clair sur internet (cf. jalon M2).
 *
 * Identique au transport clair pour tout le reste :
 *   Handshake: [PAYLOAD_LEN(1B) | "token" | "token:heartbeatMs"]
 *   puis frames binaires iWidgets v1.
 *   Heartbeat piloté par la façade, backoff exponentiel + jitter.
 *
 * Validation du serveur (par défaut) : les racines Let's Encrypt
 * (ISRG Root X1 + X2) sont embarquées → l'ESP32 vérifie l'identité
 * du serveur. Pour du hardware/débogage :
 *   - setCACert(pem) : fournir sa propre racine (serveur self-hosted).
 *   - setInsecure()  : chiffrer SANS vérifier l'identité (déconseillé
 *                      en prod — MITM possible ; utile en bring-up).
 *
 * Côté serveur (M2), un portier TLS (caddy-l4) termine le TLS sur le
 * port 9443 et forwarde vers le relais en interne.
 *
 * Copyright (c) 2025 InstantIoT — MIT License
 * ============================================================
 */

#if !defined(ARDUINO_ARCH_ESP32) && !defined(ESP32)
#  error "TlsClient_ESP32.hpp requires ESP32"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "../../core/Transport.h"
#include "RaisonWiFi_ESP32.hpp"
#include "../../InstantIoTConfig.h"
#include "../../certs/InstantIoT_LE_Roots.h"

#ifndef INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS 15000
#endif

#ifndef INSTANTIOT_TCP_CONNECT_TIMEOUT_MS
  #define INSTANTIOT_TCP_CONNECT_TIMEOUT_MS 5000
#endif

// Handshake TLS : borne le temps de négociation (secondes) pour éviter
// qu'une connexion muette bloque le boot du device.
#ifndef INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S
  #define INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S 10
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MIN_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MIN_MS 1000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_MAX_MS
  #define INSTANTIOT_RECONNECT_BACKOFF_MAX_MS 30000
#endif

#ifndef INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT
  #define INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT 25
#endif

namespace iiot {

class TlsClient_ESP32 : public ITransport {
public:

    TlsClient_ESP32(
        const char* serverIp,
        uint16_t    serverPort,
        const char* token
    ) : serverIp_(serverIp)
      , serverPort_(serverPort)
      , token_(token)
      , ssid_(nullptr)
      , pass_(nullptr)
      , caCert_(INSTANTIOT_LE_ROOT_CAS)  // validation par défaut (Let's Encrypt)
      , insecure_(false)
      , nextRetryAt_(0)
      , backoffMs_(INSTANTIOT_RECONNECT_BACKOFF_MIN_MS)
      , heartbeatMs_(0)
    {}

    // ============================================================
    // 💓 Heartbeat — appelé par la façade avant begin()
    // ============================================================
    void setHeartbeat(uint32_t intervalMs) {
        heartbeatMs_ = intervalMs;
    }

    uint32_t getHeartbeat() const { return heartbeatMs_; }

    // ============================================================
    // 🔑 WiFi credentials — appelé par la façade avant begin()
    // ============================================================
    void setCredentials(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
    }

    // ============================================================
    // 🔐 Confiance TLS — appeler AVANT begin()
    // ============================================================
    //
    // setCACert : fournir sa propre racine (ex. serveur self-hosted
    //   avec un cert maison). Remplace les racines Let's Encrypt.
    void setCACert(const char* pem) {
        caCert_   = pem;
        insecure_ = false;
    }

    // setInsecure : chiffre SANS vérifier l'identité du serveur.
    //   ⚠️ MITM possible — bring-up/débogage uniquement, pas la prod.
    void setInsecure() {
        insecure_ = true;
    }

    // ============================================================
    // 🔧 LIFECYCLE
    // ============================================================

    bool begin() override {
        if (!ssid_ || !pass_) {
            IIOT_LOG("[WiFiSecure] Missing WiFi credentials");
            return false;
        }

        if (!connectWiFi()) return false;
        if (!connectServer()) return false;

        backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
        return true;
    }

    void poll() override {
        // Sans identifiants, il n'y a rien a retenter — et `WiFi.begin(nullptr)`
        // ne pardonne pas. Le garde-fou vivait dans `begin()` seul ; depuis que
        // `loop()` fait tourner `poll()` meme apres un `begin()` rate, il doit
        // etre ici aussi.
        if (!ssid_ || !pass_) return;

        // ── Le WiFi n'est pas la ────────────────────────────────
        //
        // Une association peut etre EN COURS. Relancer `WiFi.begin()` a ce
        // moment-la ne la relance pas : elle la TUE et la fait repartir de
        // zero. L'ESP32 le dit lui-meme —
        //
        //     E (86789) wifi:sta is connecting, cannot set config
        //
        // — et une carte sur un reseau lent n'arrive alors jamais : chaque
        // reprise l'interrompt juste avant qu'elle n'aboutisse. La pile
        // continue toute seule ; il suffit de ne plus lui couper la parole.
        if (WiFi.status() != WL_CONNECTED) {
            client_.stop();

            // La puce sait POURQUOI, et elle le sait tout de suite : le refus
            // d'une cle arrive en deux secondes. Le dire ici plutot qu'a
            // l'expiration du delai, c'est treize secondes de moins a se
            // demander ce qui se passe. Une fois par raison, pas par essai.
            diLaRaisonWiFi();

            if (tentativeWiFiDepuis_ != 0) {
                // Une tentative est en vol : on regarde, on ne touche pas.
                if (millis() - tentativeWiFiDepuis_ < INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS)
                    return;
                // Elle a assez dure. On la coupe proprement — sans
                // `disconnect()`, le `begin()` suivant retombe sur la meme
                // erreur — et on laisse le backoff decider du moment.
                IIOT_LOG("[WiFiSecure] WiFi attempt timed out — will retry");
                WiFi.disconnect();
                tentativeWiFiDepuis_ = 0;
                scheduleRetry();
                return;
            }

            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiSecure] WiFi reconnect attempt #", retryAttempt_);
            lanceLaTentativeWiFi();
            return;   // on rendra la main a la prochaine passe
        }

        // Le WiFi est la : plus rien en vol, et la raison precedente
        // n'a plus cours.
        tentativeWiFiDepuis_ = 0;
        oublieLaRaisonWiFi();

        // TLS/TCP tombé → reconnexion (avec backoff)
        if (!client_.connected()) {
            client_.stop();
            if (millis() < nextRetryAt_) return;

            retryAttempt_++;
            IIOT_LOG_VAL("[WiFiSecure] TLS reconnect attempt #", retryAttempt_);
            if (!connectServer()) {
                scheduleRetry();
                return;
            }
            backoffMs_ = INSTANTIOT_RECONNECT_BACKOFF_MIN_MS;
            retryAttempt_ = 0;
        }
    }

    // ============================================================
    // 📡 STATUS
    // ============================================================

    bool connected() override {
        return WiFi.status() == WL_CONNECTED && client_.connected();
    }

    int available() override {
        return connected() ? client_.available() : 0;
    }

    // ============================================================
    // 📥 READ
    // ============================================================

    int read(uint8_t* buf, size_t len) override {
        if (!connected()) return -1;
        return client_.read(buf, len);
    }

    // ============================================================
    // 📤 WRITE
    // ============================================================

    size_t write(const uint8_t* buf, size_t len) override {
        if (!connected()) return 0;
        return client_.write(buf, len);
    }

    // ============================================================
    // 🔎 GETTERS
    // ============================================================

    IPAddress   getLocalIP()  const { return WiFi.localIP(); }
    const char* getServerIP() const { return serverIp_; }
    uint16_t    getPort()     const { return serverPort_; }
    bool        isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }

private:

    /** Le SEUL endroit qui appelle `WiFi.begin`, et il note l'heure. */
    void lanceLaTentativeWiFi() {
        ecouteLesRaisonsWiFi();
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_, pass_);
        tentativeWiFiDepuis_ = millis();
    }

    // ----- WiFi -----
    bool connectWiFi() {
        IIOT_LOG_VAL("[WiFiSecure] WiFi connecting to: ", ssid_);

        lanceLaTentativeWiFi();

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > INSTANTIOT_WIFI_CONNECT_TIMEOUT_MS) {
                // La tentative reste EN VOL : la pile continue d'essayer, et
                // `poll()` la laissera aboutir plutot que de la relancer.
                IIOT_LOG("[WiFiSecure] WiFi timeout — la tentative continue en fond");
                return false;
            }
            delay(100);
        }

        tentativeWiFiDepuis_ = 0;
        oublieLaRaisonWiFi();
        IIOT_LOG_VAL("[WiFiSecure] WiFi OK - IP: ", WiFi.localIP().toString().c_str());
        return true;
    }

    // ----- TLS + TCP + handshake -----
    bool connectServer() {
        IIOT_LOG_2("[WiFiSecure] TLS connecting: ", serverIp_, ":", serverPort_);

        // Confiance TLS — configurée AVANT connect() (chaque tentative).
        if (insecure_) {
            client_.setInsecure();
            IIOT_LOG("[WiFiSecure] ⚠️ INSECURE mode — server identity NOT verified");
        } else {
            client_.setCACert(caCert_);
        }
        client_.setHandshakeTimeout(INSTANTIOT_TLS_HANDSHAKE_TIMEOUT_S);
        client_.setTimeout(INSTANTIOT_TCP_CONNECT_TIMEOUT_MS);

        if (!client_.connect(serverIp_, serverPort_)) {
            IIOT_LOG("[WiFiSecure] TLS connect FAILED (check CA / port / SNI)");
            return false;
        }

        // Handshake applicatif : [PAYLOAD_LEN | PAYLOAD_BYTES]
        //   payload = "token"           (legacy, heartbeatMs_ = 0)
        //   payload = "token:heartbeat"  (heartbeat activé)
        if (!token_) {
            IIOT_LOG("[WiFiSecure] Missing device token");
            client_.stop();
            return false;
        }

        char payload[288];
        int written = 0;
        if (heartbeatMs_ > 0) {
            written = snprintf(payload, sizeof(payload), "%s:%lu",
                               token_, (unsigned long)heartbeatMs_);
        } else {
            written = snprintf(payload, sizeof(payload), "%s", token_);
        }
        if (written <= 0 || written > 255) {
            IIOT_LOG("[WiFiSecure] Invalid handshake payload length");
            client_.stop();
            return false;
        }

        uint8_t lenByte = (uint8_t)written;
        if (client_.write(&lenByte, 1) != 1 ||
            client_.write(reinterpret_cast<const uint8_t*>(payload), written) != (size_t)written) {
            IIOT_LOG("[WiFiSecure] Handshake write FAILED");
            client_.stop();
            return false;
        }

        IIOT_LOG_VAL("[WiFiSecure] Handshake sent (TLS), heartbeat=", (long)heartbeatMs_);
        return true;
    }

    // ----- Backoff avec jitter (identique au transport clair) -----
    void scheduleRetry() {
        uint32_t base = backoffMs_;
        int32_t jitterRange = (int32_t)(base * INSTANTIOT_RECONNECT_BACKOFF_JITTER_PCT) / 100;
        int32_t jitter = (jitterRange > 0) ? (int32_t)random(-jitterRange, jitterRange + 1) : 0;
        int32_t actualDelay = (int32_t)base + jitter;
        if (actualDelay < 100) actualDelay = 100;

        nextRetryAt_ = millis() + (uint32_t)actualDelay;

        IIOT_LOG_2("[WiFiSecure] Next retry in ", actualDelay, "ms (base ", base);

        uint32_t next = base * 2;
        if (next >= INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
            if (base < INSTANTIOT_RECONNECT_BACKOFF_MAX_MS) {
                IIOT_LOG_VAL(
                    "[WiFiSecure] Reached max backoff — persistent issue, attempt #",
                    retryAttempt_
                );
            }
            next = INSTANTIOT_RECONNECT_BACKOFF_MAX_MS;
        }
        backoffMs_ = next;
    }

    const char* serverIp_;
    uint16_t    serverPort_;
    const char* token_;
    const char* ssid_;
    const char* pass_;
    const char* caCert_;            // racine(s) de confiance (PEM)
    bool        insecure_;          // true = chiffre sans vérifier l'identité

    WiFiClientSecure client_;
    uint32_t    nextRetryAt_;
    uint32_t    backoffMs_;
    uint32_t    retryAttempt_ = 0;
    /** Heure du dernier `WiFi.begin`, ou 0 si rien n'est en vol. */
    uint32_t    tentativeWiFiDepuis_ = 0;
    uint32_t    heartbeatMs_;
};

} // namespace iiot
