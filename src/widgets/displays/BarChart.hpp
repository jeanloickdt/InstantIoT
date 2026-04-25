#pragma once
#include <Arduino.h>
#include "../WidgetBase.hpp"
#include "../../core/BinaryCodec.hpp"

namespace InstantIoT {

/**
 * BarChart — affichage de N valeurs sous forme de barres.
 *
 * **Direction unique** : Arduino → App. Le widget est purement
 * descendant : l'ESP32 pousse les valeurs, l'app les affiche. Les
 * labels et couleurs des barres sont configurés **côté app**
 * (settings sheet) — l'utilisateur définit le nombre de "slots" et
 * leur look, et le sketch envoie un array de N valeurs dans le
 * même ordre.
 *
 * Exemple typique :
 *   float values[3];
 *   values[0] = readTemperature();
 *   values[1] = readHumidity();
 *   values[2] = readPressure();
 *   instant.barChart("env").setValues(values, 3);
 *
 * Mémoire : ~16 octets par instance + 4×count en buffer pour
 * setValues. Pas d'allocation dynamique.
 */
class BarChartWidget : public DisplayWidget {
public:
    BarChartWidget(const char* id, IMessageSender& sender)
        : DisplayWidget(id, sender) {}

    uint8_t getTypeCode() const override { return TYPE_BARCHART; }

    /**
     * Pousse un array complet de valeurs. L'app remplace toutes
     * les barres connues par ces valeurs (ordre = ordre des slots
     * configurés dans le settings sheet).
     *
     * @param values pointeur vers `count` floats
     * @param count nombre de valeurs (doit être ≥ 1, max 64
     *              pour tenir dans le buffer 256B du codec)
     */
    BarChartWidget& setValues(const float* values, uint8_t count) {
        if (count == 0 || values == nullptr) return *this;
        if (count > 64) count = 64;  // cap pour rester sous buffer

        uint8_t buf[1 + 64 * 4];
        buf[0] = count;
        for (uint8_t i = 0; i < count; i++) {
            writeFloatLE(buf + 1 + i * 4, values[i]);
        }
        sendBinary(EV_BAR_SETVALUES, buf, 1 + count * 4);
        return *this;
    }

    /**
     * Met à jour une **seule** barre par son index (0-indexé).
     * Plus économe en bande passante que `setValues` quand une seule
     * valeur change. Si `index` ne correspond à aucun slot configuré
     * côté app, la frame est ignorée silencieusement.
     */
    BarChartWidget& setBar(uint8_t index, float value) {
        uint8_t buf[5];
        buf[0] = index;
        writeFloatLE(buf + 1, value);
        sendBinary(EV_BAR_SETBAR, buf, 5);
        return *this;
    }

    /**
     * Reset toutes les barres à 0 côté app. Utile lors d'un boot
     * propre ou d'un reset utilisateur.
     */
    BarChartWidget& clear() {
        sendBinary(EV_BAR_CLEAR);
        return *this;
    }
};

} // namespace InstantIoT
