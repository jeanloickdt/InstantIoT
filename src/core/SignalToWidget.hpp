#pragma once
#include <Arduino.h>
#include "InstantIoTMessage.hpp"

/**
 * Ce qu'un signal veut dire pour un widget.
 *
 * L'app n'envoie plus d'événements de widget : elle écrit un SIGNAL, à une
 * adresse, et le contenu est un nombre ou du texte. C'est ici qu'on retrouve
 * le sens — pour que le croquis lise `WHEN_PRESSED` et `x, y`, et non des
 * chiffres et des chaînes à découper.
 *
 * Écrit une fois, ici, plutôt que dans chaque croquis. Une bibliothèque
 * existe pour ça.
 */
namespace InstantIoT {

/**
 * La convention des gestes : **1 appui, 0 relâchement, 2 appui long**.
 *
 * Elle est déclarée côté app (`CommandAsValue.kt`) et lue ici. Les deux
 * moitiés se lisent ensemble : changer un chiffre là-bas change ce que les
 * blocs reçoivent ici.
 *
 * Ce sont aussi les valeurs par défaut des trois réglages d'un bouton —
 * l'utilisateur peut les changer, il part de ce que la carte sait lire.
 */
inline ButtonEventKind gestureFromValue(float v) {
    if (v >= 1.5f) return ButtonEventKind::LongPress;   // 2
    if (v >= 0.5f) return ButtonEventKind::Press;       // 1
    return ButtonEventKind::Release;                    // 0
}

/**
 * Une position de manette, écrite `"0.42,-0.15"`.
 *
 * Deux nombres au centième dans un seul texte, parce qu'une position n'est
 * pas deux valeurs indépendantes : les écrire séparément ferait deux trames
 * pour un seul geste, sans ordre garanti entre elles.
 *
 * @return faux si le texte n'a pas cette forme — le bloc n'est alors pas
 *         appelé, plutôt que d'être appelé avec des zéros inventés.
 */
inline bool decodePosition(const char* texte, float& x, float& y) {
    if (!texte) return false;
    const char* virgule = strchr(texte, ',');
    if (!virgule || virgule == texte || *(virgule + 1) == '\0') return false;
    x = (float)atof(texte);
    y = (float)atof(virgule + 1);
    return true;
}

/** Le mot d'une touche, sans son suffixe de geste. */
inline DPadButton padFromName(const char* mot, size_t len) {
    struct Paire { const char* mot; DPadButton touche; };
    static const Paire TABLE[] = {
        {"UP", DPadButton::Up}, {"DOWN", DPadButton::Down},
        {"LEFT", DPadButton::Left}, {"RIGHT", DPadButton::Right},
        {"CENTER", DPadButton::Center},
        {"A", DPadButton::A}, {"B", DPadButton::B},
        {"X", DPadButton::X}, {"Y", DPadButton::Y},
        {"TRIANGLE", DPadButton::Triangle}, {"CIRCLE", DPadButton::Circle},
        {"SQUARE", DPadButton::Square}, {"CROSS", DPadButton::Cross},
    };
    for (const Paire& p : TABLE) {
        if (strlen(p.mot) == len && strncmp(p.mot, mot, len) == 0) return p.touche;
    }
    return DPadButton::Unknown;
}

/**
 * Une touche de croix, écrite `"UP"`, `"UP_LONG"` ou `"UP_RELEASE"`.
 *
 * Le geste est DANS le mot, et c'est délibéré : sans le suffixe, un appui
 * long écrivait `UP` comme un appui court — le même mot deux fois à 400 ms
 * d'écart, et la carte incapable de distinguer un tap d'un maintien.
 *
 * @return faux si le mot ne nomme aucune touche connue.
 */
inline bool decodePad(const char* mot, DPadButton& touche, DPadEventKind& geste) {
    if (!mot) return false;
    size_t n = strlen(mot);

    geste = DPadEventKind::Press;
    const char* SUFFIXE_LONG = "_LONG";
    const char* SUFFIXE_REL  = "_RELEASE";
    size_t nLong = strlen(SUFFIXE_LONG), nRel = strlen(SUFFIXE_REL);

    if (n > nLong && strcmp(mot + n - nLong, SUFFIXE_LONG) == 0) {
        geste = DPadEventKind::LongPress; n -= nLong;
    } else if (n > nRel && strcmp(mot + n - nRel, SUFFIXE_REL) == 0) {
        geste = DPadEventKind::Release;  n -= nRel;
    }

    touche = padFromName(mot, n);
    return touche != DPadButton::Unknown;
}

/** Le nom d'une touche, pour un croquis qui veut l'imprimer. */
inline const char* padName(DPadButton t) {
    switch (t) {
        case DPadButton::Up:       return "up";
        case DPadButton::Down:     return "down";
        case DPadButton::Left:     return "left";
        case DPadButton::Right:    return "right";
        case DPadButton::Center:   return "center";
        case DPadButton::A:        return "a";
        case DPadButton::B:        return "b";
        case DPadButton::X:        return "x";
        case DPadButton::Y:        return "y";
        case DPadButton::Triangle: return "triangle";
        case DPadButton::Circle:   return "circle";
        case DPadButton::Square:   return "square";
        case DPadButton::Cross:    return "cross";
        default:                   return "unknown";
    }
}

} // namespace InstantIoT
