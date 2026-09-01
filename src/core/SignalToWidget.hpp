#pragma once
#include <Arduino.h>
#include "InstantIoTMessage.hpp"

/**
 * What a signal means to a block.
 *
 * The app no longer sends widget events: it writes a SIGNAL, at an
 * address, and the content is a number or text. This is where the meaning
 * comes back — so the sketch reads `WHEN_PRESSED` and `x, y`, not digits
 * and strings to slice up.
 *
 * Written once, here, rather than in every sketch. That is what a library
 * is for.
 */
namespace iiot {

/**
 * The gesture convention: **1 press, 0 release, 2 long press**.
 *
 * It is declared on the app side (`CommandAsValue.kt`) and read here. The
 * two halves are read together: changing a digit there changes what the
 * blocks receive here.
 *
 * They are also the defaults of a button's three settings — the user can
 * change them, starting from what the board knows how to read.
 */
inline ButtonEventKind gestureFromValue(float v) {
    if (v >= 1.5f) return ButtonEventKind::LongPress;   // 2
    if (v >= 0.5f) return ButtonEventKind::Press;       // 1
    return ButtonEventKind::Release;                    // 0
}

/**
 * A joystick position, written `"0.42,-0.15"`.
 *
 * Two numbers to the hundredth in a single text, because a position is not
 * two independent values: writing them separately would make two frames
 * for one gesture, with no guaranteed order between them.
 *
 * @return false if the text does not have that shape — the block is then
 *         not called, rather than called with invented zeros.
 */
inline bool decodePosition(const char* texte, float& x, float& y) {
    if (!texte) return false;
    const char* virgule = strchr(texte, ',');
    if (!virgule || virgule == texte || *(virgule + 1) == '\0') return false;
    x = (float)atof(texte);
    y = (float)atof(virgule + 1);
    return true;
}

/** A key's word, without its gesture suffix. */
inline DPadButton padFromName(const char* word, size_t len) {
    struct Pair { const char* word; DPadButton key; };
    static const Pair TABLE[] = {
        {"UP", DPadButton::Up}, {"DOWN", DPadButton::Down},
        {"LEFT", DPadButton::Left}, {"RIGHT", DPadButton::Right},
        {"CENTER", DPadButton::Center},
        {"A", DPadButton::A}, {"B", DPadButton::B},
        {"X", DPadButton::X}, {"Y", DPadButton::Y},
        {"TRIANGLE", DPadButton::Triangle}, {"CIRCLE", DPadButton::Circle},
        {"SQUARE", DPadButton::Square}, {"CROSS", DPadButton::Cross},
    };
    for (const Pair& p : TABLE) {
        if (strlen(p.word) == len && strncmp(p.word, word, len) == 0) return p.key;
    }
    return DPadButton::Unknown;
}

/**
 * Une key de croix, écrite `"UP"`, `"UP_LONG"` ou `"UP_RELEASE"`.
 *
 * The gesture is IN the word, and that is deliberate: without the suffix,
 * a long press wrote `UP` exactly like a short one — the same word twice,
 * 400 ms apart, and the board unable to tell a tap from a hold.
 *
 * @return false if the word names no known key.
 */
inline bool decodePad(const char* word, DPadButton& key, DPadEventKind& gesture) {
    if (!word) return false;
    size_t n = strlen(word);

    gesture = DPadEventKind::Press;
    const char* SUFFIXE_LONG = "_LONG";
    const char* SUFFIXE_REL  = "_RELEASE";
    size_t nLong = strlen(SUFFIXE_LONG), nRel = strlen(SUFFIXE_REL);

    if (n > nLong && strcmp(word + n - nLong, SUFFIXE_LONG) == 0) {
        gesture = DPadEventKind::LongPress; n -= nLong;
    } else if (n > nRel && strcmp(word + n - nRel, SUFFIXE_REL) == 0) {
        gesture = DPadEventKind::Release;  n -= nRel;
    }

    key = padFromName(word, n);
    return key != DPadButton::Unknown;
}

/** A key's name, for a sketch that wants to print it. */
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

} // namespace iiot
