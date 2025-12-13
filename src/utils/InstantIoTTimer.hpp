#pragma once
/**
 * ============================================================
 * ⏱️ InstantIoTTimer.hpp - Gestionnaire de timers non-bloquants
 * ============================================================
 * 
 * Usage:
 *   InstantTimer timers;
 *   
 *   void setup() {
 *       timers.every(1000, []{ Serial.println("Chaque seconde"); });
 *       timers.once(5000, []{ Serial.println("Une seule fois"); });
 *       timers.times(500, []{ Serial.println("3 fois"); }, 3);
 *   }
 *   
 *   void loop() {
 *       timers.run();
 *   }
 * 
 * ============================================================
 */

#include <Arduino.h>

#ifndef INSTANT_TIMERS_MAX
  #define INSTANT_TIMERS_MAX 16
#endif

#ifndef INSTANT_DEBUG
  #define INSTANT_DEBUG 0
#endif

class InstantTimer {
public:
    using Fn = void (*)();

    // ============================================================
    // 🎯 API FLUIDE POUR PLANIFICATION
    // ============================================================
    
    /**
     * Exécute une fonction à intervalles réguliers (infini)
     * @param ms Intervalle en millisecondes
     * @param fn Fonction à appeler
     * @return ID du timer (-1 si erreur)
     */
    int every(uint32_t ms, Fn fn) { 
        return schedule(ms, fn, 0, true); 
    }
    
    /**
     * Exécute une fonction une seule fois après un délai
     * @param ms Délai en millisecondes
     * @param fn Fonction à appeler
     * @return ID du timer (-1 si erreur)
     */
    int once(uint32_t ms, Fn fn) { 
        return schedule(ms, fn, 1, true); 
    }
    
    /**
     * Exécute une fonction N fois à intervalles réguliers
     * @param ms Intervalle en millisecondes
     * @param fn Fonction à appeler
     * @param n Nombre de répétitions
     * @return ID du timer (-1 si erreur)
     */
    int times(uint32_t ms, Fn fn, uint16_t n) { 
        return schedule(ms, fn, n, true); 
    }

    // ============================================================
    // 🔧 CONTRÔLE DES TIMERS
    // ============================================================
    
    /**
     * Active ou désactive un timer
     */
    bool enable(int id, bool en = true) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ Enable impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalide"));
#endif
            return false;
        }
        tasks_[id].active = en;
#if INSTANT_DEBUG
        Serial.print(F("[Timer] "));
        Serial.print(en ? F("✅ Activé") : F("⏸️ Désactivé"));
        Serial.print(F(" timer id="));
        Serial.println(id);
#endif
        return true;
    }

    /**
     * Désactive un timer (raccourci)
     */
    bool disable(int id) { 
        return enable(id, false); 
    }

    /**
     * Annule et libère un timer
     */
    bool cancel(int id) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ Cancel impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalide"));
#endif
            return false;
        }
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ❌ Timer id="));
        Serial.print(id);
        Serial.println(F(" annulé"));
#endif
        tasks_[id] = Task{};
        return true;
    }

    /**
     * Change l'intervalle d'un timer existant
     */
    bool changeInterval(int id, uint32_t ms) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ ChangeInterval impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalide"));
#endif
            return false;
        }
        tasks_[id].interval = ms;
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ⏱️ Timer id="));
        Serial.print(id);
        Serial.print(F(" → nouvel interval="));
        Serial.print(ms);
        Serial.println(F(" ms"));
#endif
        return true;
    }

    /**
     * Force l'exécution immédiate d'un timer
     */
    bool executeNow(int id) {
        if (!valid(id) || !tasks_[id].inUse) {
#if INSTANT_DEBUG
            Serial.print(F("[Timer] ⚠️ ExecuteNow impossible: id="));
            Serial.print(id);
            Serial.println(F(" invalide"));
#endif
            return false;
        }
        tasks_[id].nextAt = millis();
#if INSTANT_DEBUG
        Serial.print(F("[Timer] ⚡ Exécution immédiate forcée: id="));
        Serial.println(id);
#endif
        return true;
    }

    // ============================================================
    // 🔄 BOUCLE PRINCIPALE
    // ============================================================
    
    /**
     * Exécute les timers - appeler dans loop()
     */
    void run() {
        if (running_) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] 🔒 Réentrance bloquée"));
#endif
            return; // 🛡️ Anti-réentrance
        }
        
        running_ = true;

        const uint32_t now = millis();
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            Task& t = tasks_[i];
            
            if (!t.inUse || !t.active || !t.fn) continue;
            if ((int32_t)(now - t.nextAt) < 0) continue;

            // ⏰ Planifier la prochaine exécution
            if (t.interval > 0) {
                do { 
                    t.nextAt += t.interval; 
                } while ((int32_t)(now - t.nextAt) >= 0);
            } else {
                t.nextAt = now + 1; // 🛡️ Sécurité si interval==0
            }

#if INSTANT_DEBUG
            Serial.print(F("[Timer] 🔔 Exécution timer id="));
            Serial.println(i);
#endif

            Fn callback = t.fn;
            callback();

            // 📉 Décrémenter et libérer si terminé
            if (t.remaining > 0 && --t.remaining == 0) {
#if INSTANT_DEBUG
                Serial.print(F("[Timer] ✅ Timer id="));
                Serial.print(i);
                Serial.println(F(" terminé"));
#endif
                t = Task{};
            }
        }

        running_ = false;
    }

    // ============================================================
    // 📊 STATISTIQUES
    // ============================================================
    
    /**
     * Nombre de timers actifs
     */
    int activeCount() const {
        int count = 0;
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (tasks_[i].inUse && tasks_[i].active) count++;
        }
        return count;
    }
    
    /**
     * Nombre de slots utilisés
     */
    int usedCount() const {
        int count = 0;
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (tasks_[i].inUse) count++;
        }
        return count;
    }
    
    /**
     * Nombre de slots libres
     */
    int freeCount() const {
        return INSTANT_TIMERS_MAX - usedCount();
    }

private:
    struct Task {
        Fn       fn        = nullptr;
        uint32_t interval  = 0;
        uint32_t nextAt    = 0;
        uint16_t remaining = 0;   // 0 = infini
        bool     active    = false;
        bool     inUse     = false;
    };

    Task tasks_[INSTANT_TIMERS_MAX];
    bool running_ = false; // 🔒 Protection boucles imbriquées

    int schedule(uint32_t ms, Fn fn, uint16_t repeat, bool en) {
        if (!fn) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ❌ Schedule impossible: callback null"));
#endif
            return -1;
        }

        if (ms == 0) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ⚠️ Schedule avec ms=0, forcé à 1ms"));
#endif
            ms = 1; // 🛡️ Sécurité
        }

        const int id = firstFree();
        if (id < 0) {
#if INSTANT_DEBUG
            Serial.println(F("[Timer] ❌ Plus de slots disponibles!"));
#endif
            return -1;
        }

        tasks_[id].fn        = fn;
        tasks_[id].interval  = ms;
        tasks_[id].nextAt    = millis() + ms;
        tasks_[id].remaining = repeat;
        tasks_[id].active    = en;
        tasks_[id].inUse     = true;

#if INSTANT_DEBUG
        Serial.print(F("[Timer] ➕ Timer créé: id="));
        Serial.print(id);
        Serial.print(F(" interval="));
        Serial.print(ms);
        Serial.print(F(" ms repeat="));
        Serial.println(repeat);
#endif

        return id;
    }

    int firstFree() const {
        for (int i = 0; i < INSTANT_TIMERS_MAX; i++) {
            if (!tasks_[i].inUse) return i;
        }
        return -1;
    }

    static inline bool valid(int id) { 
        return id >= 0 && id < INSTANT_TIMERS_MAX; 
    }
};
