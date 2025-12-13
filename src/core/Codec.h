#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * Codec.h - Interface abstraite pour encodage/décodage messages
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <stdint.h>
#include <stddef.h>

namespace InstantIoT {

/**
 * @brief Structure représentant un message décodé
 */
struct DecodedMessage {
    const char* dashboardId;    // ID du dashboard
    const char* deviceId;       // ID du device
    const char* widgetId;       // ID du widget
    const char* widgetType;     // Type du widget (simplebutton, led, etc.)
    const char* event;          // Event (press, release, setvalue, etc.)
    
    // Payload décodé (jusqu'à 8 paramètres)
    struct Param {
        const char* key;
        const char* value;
    };
    Param params[8];
    uint8_t paramCount;
    
    // Helpers pour accéder aux paramètres
    const char* getParam(const char* key) const {
        for (uint8_t i = 0; i < paramCount; i++) {
            if (strcmp(params[i].key, key) == 0) {
                return params[i].value;
            }
        }
        return nullptr;
    }
    
    float getParamFloat(const char* key, float defaultValue = 0.0f) const {
        const char* val = getParam(key);
        return val ? atof(val) : defaultValue;
    }
    
    int getParamInt(const char* key, int defaultValue = 0) const {
        const char* val = getParam(key);
        return val ? atoi(val) : defaultValue;
    }
    
    bool getParamBool(const char* key, bool defaultValue = false) const {
        const char* val = getParam(key);
        if (!val) return defaultValue;
        return (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
    }
};

/**
 * @brief Interface abstraite pour les codecs
 * 
 * Permet d'encoder et décoder les messages InstantIoT.
 * L'implémentation par défaut utilise JSON (JsonCodec).
 */
class Codec {
public:
    virtual ~Codec() = default;
    
    /**
     * @brief Encode un message vers le buffer de sortie
     * 
     * @param buffer Buffer de sortie
     * @param bufferSize Taille du buffer
     * @param dashboardId ID du dashboard
     * @param deviceId ID du device
     * @param widgetId ID du widget (ex: "led1")
     * @param widgetType Type du widget (ex: "led")
     * @param event Événement (ex: "turnon", "setvalue")
     * @param payload Payload JSON optionnel (ex: "{\"value\":42}")
     * @return size_t Nombre de bytes écrits, 0 si erreur
     */
    virtual size_t encode(
        char* buffer,
        size_t bufferSize,
        const char* dashboardId,
        const char* deviceId,
        const char* widgetId,
        const char* widgetType,
        const char* event,
        const char* payload = nullptr
    ) = 0;
    
    /**
     * @brief Décode un message depuis le buffer d'entrée
     * 
     * @param buffer Buffer d'entrée contenant le message
     * @param length Longueur du message
     * @param outMessage Structure de sortie pour le message décodé
     * @return true Si décodage réussi
     * @return false Si erreur de décodage
     */
    virtual bool decode(
        const char* buffer,
        size_t length,
        DecodedMessage& outMessage
    ) = 0;
    
    /**
     * @brief Retourne le Content-Type pour ce codec
     * @return const char* Content-Type (ex: "application/json")
     */
    virtual const char* contentType() const = 0;
};

} // namespace InstantIoT
