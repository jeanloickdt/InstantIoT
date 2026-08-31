#pragma once
/**
 * ============================================================
 *  InstantIoTWhen.hpp — un bloc par ADRESSE
 * ============================================================
 *
 * Un bloc vise une adresse — `I0`, `I2` — et non un nom de widget. L'app
 * n'envoie plus d'evenements de widget : elle ecrit un SIGNAL, et ces blocs
 * le decodent.
 *
 * ## La regle de forme
 *
 * **La tete porte la donnee, `WHEN_` porte le genre.**
 *
 * Un seul genre d'evenement — une position, une valeur — et les parametres
 * suffisent :
 *
 *   IJoystick(I2, float x, float y) { driveMotors(x, y); }
 *   IHorizontalSlider(I1, float v)  { analogWrite(LED_PIN, v); }
 *   ISwitch(I3, bool on)            { digitalWrite(RELAY_PIN, on); }
 *   ISegmentedSwitch(I4, int index) { mode = index; }
 *
 * Plusieurs genres, et `WHEN_` les trie — ce qui evite l'escalier de `if` :
 *
 *   ISimpleButton(I0) {
 *       WHEN_PRESSED      { Serial.println("press"); }
 *       WHEN_RELEASED     { Serial.println("release"); }
 *       WHEN_LONG_PRESSED { Serial.println("hold"); }
 *   };
 *
 *   IDirectionPad(I5) {
 *       WHEN_UP      { avancer(); }
 *       WHEN_UP_LONG { accelerer(); }
 *       WHEN_PAD_PRESSED(btn) { moteur(btn); }   // les cinq d'un coup
 *   };
 *
 * ## Ce que la carte lit
 *
 * Un bouton ecrit une VALEUR, et le geste y est par convention : **1 appui,
 * 0 relachement, 2 appui long**. Une manette ecrit `"0.42,-0.15"`, une croix
 * ecrit `"UP"`, `"UP_LONG"`, `"UP_RELEASE"`. Le decodage vit dans
 * `SignalToWidget.hpp`, ecrit une fois plutot que dans chaque croquis.
 *
 * ## Plusieurs blocs, une adresse
 *
 * Des blocs de types differents peuvent viser la meme adresse — un
 * `ISignal(I0, float v)` a cote d'un `ISimpleButton(I0)`. Les deux sont
 * appeles, sauf pour un RAPPEL : au redemarrage, le serveur renvoie la
 * derniere valeur, et seul `ISignal` la recoit. Un etat se rappelle, un geste
 * ne se rejoue pas.
 *
 * Chaque bloc coute un maillon de liste chainee, sans allocation.
 *
 * ## Le point-virgule final
 *
 * Le bloc se ferme par `};` — le `;` termine la definition du registrar que
 * la macro engendre.
 *
 * ============================================================
 */

#include <Arduino.h>
#include "../core/SignalToWidget.hpp"
#include "../core/InstantIoTMessage.hpp"

// ============================================================
// 🔧 HELPERS token-pasting
// ============================================================
#ifndef _IIO_CAT
#define _IIO_CAT(a, b)  a##b
#define _IIO_XCAT(a, b) _IIO_CAT(a, b)
#define _IIO_UID(base)  _IIO_XCAT(base, __COUNTER__)
#endif

// ============================================================
// 🎯 I<Widget>(Ix) — un bloc, à une ADRESSE
//
// Ces blocs écoutaient un canal que plus personne n'alimente : ils étaient
// enregistrés par NOM, sur des événements de widget que l'app n'envoie plus.
// Elle écrit un SIGNAL, à une adresse, et le contenu est un nombre ou du
// texte. Ils l'écoutent donc, et le décodent — c'est tout ce qui change.
//
// Le vocabulaire ne bouge pas : `WHEN_PRESSED` reçoit ce qu'il recevait, et
// un croquis ne change que `"btn1"` en `I0`.
//
// **La tête porte la donnée, `WHEN_` porte le genre.** Un seul genre
// d'événement — une position, une valeur — et les paramètres suffisent.
// Plusieurs genres, et `WHEN_` les trie, ce qui évite l'escalier de `if`.
// ============================================================

// ── Un seul genre : la donnée dans la tête ───────────────────

/** `IJoystick(I2, float x, float y) { … }` — l'app écrit `"0.42,-0.15"`. */
#define IJoystick(ref, ...) \
    _IIO_IJOY(ref, _IIO_UID(_iioJyF_), _IIO_UID(_iioJyT_), _IIO_UID(_iioJyR_), __VA_ARGS__)
#define _IIO_IJOY(ref, FN, TRAMP, REG, ...)                                \
    static void FN(__VA_ARGS__);                                           \
    static void TRAMP(const InstantIoT::SignalEvent& e) {                  \
        float _iioX = 0.0f, _iioY = 0.0f;                                  \
        if (InstantIoT::decodePosition(e.value.text(), _iioX, _iioY))            \
            FN(_iioX, _iioY);                                              \
    }                                                                      \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP, /* geste */ true);                   \
    static void FN(__VA_ARGS__)

/** `IHorizontalSlider(I4, float v) { … }` */
#define IHorizontalSlider(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioHsF_), _IIO_UID(_iioHsT_), _IIO_UID(_iioHsR_), __VA_ARGS__)
/** `IVerticalSlider(I5, float v) { … }` */
#define IVerticalSlider(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioVsF_), _IIO_UID(_iioVsT_), _IIO_UID(_iioVsR_), __VA_ARGS__)
/** `ISwitch(I1, bool on) { … }` — le type du paramètre fait la conversion. */
#define ISwitch(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioSwF_), _IIO_UID(_iioSwT_), _IIO_UID(_iioSwR_), __VA_ARGS__)
/** `ISegmentedSwitch(I6, int index) { … }` */
#define ISegmentedSwitch(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioSgF_), _IIO_UID(_iioSgT_), _IIO_UID(_iioSgR_), __VA_ARGS__)

// Une valeur, rendue dans le type que le bloc demande. `SignalValue` sait se
// convertir en `float`, `int` ou `bool` — c'est la déclaration du croquis qui
// choisit, pas la trame.
#define _IIO_INUM(ref, FN, TRAMP, REG, ...)                                \
    static void FN(__VA_ARGS__);                                           \
    static void TRAMP(const InstantIoT::SignalEvent& e) { FN(e.value); }   \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP, /* geste */ true);                   \
    static void FN(__VA_ARGS__)

// ── Plusieurs genres : un bloc, et `WHEN_` pour trier ────────

/** `ISimpleButton(I0) { WHEN_PRESSED … }` — 1 appui, 0 relâchement, 2 long. */
#define ISimpleButton(ref) \
    _IIO_IBTN(ref, SimpleButtonEvent, _IIO_UID(_iioSbF_), _IIO_UID(_iioSbT_), _IIO_UID(_iioSbR_))
/** `IAdvancedButton(I0) { WHEN_PRESSED … }` */
#define IAdvancedButton(ref) \
    _IIO_IBTN(ref, AdvancedButtonEvent, _IIO_UID(_iioAbF_), _IIO_UID(_iioAbT_), _IIO_UID(_iioAbR_))

#define _IIO_IBTN(ref, EVT, FN, TRAMP, REG)                                \
    static void FN(const InstantIoT::EVT&);                                \
    static void TRAMP(const InstantIoT::SignalEvent& s) {                  \
        InstantIoT::EVT e{};                                               \
        e.widgetId = "";                                                   \
        e.kind = InstantIoT::gestureFromValue((float)s.value);             \
        e.isOn = (e.kind == InstantIoT::ButtonEventKind::Press);           \
        FN(e);                                                             \
    }                                                                      \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP, /* geste */ true);                   \
    static void FN(const InstantIoT::EVT& e)

/** `IDirectionPad(I3) { WHEN_UP … }` — l'app écrit `"UP"`, `"UP_LONG"`… */
#define IDirectionPad(ref) \
    _IIO_IDP(ref, _IIO_UID(_iioDpF_), _IIO_UID(_iioDpT_), _IIO_UID(_iioDpR_))
#define _IIO_IDP(ref, FN, TRAMP, REG)                                      \
    static void FN(const InstantIoT::DirectionPadEvent&);                  \
    static void TRAMP(const InstantIoT::SignalEvent& s) {                  \
        InstantIoT::DirectionPadEvent e{};                                 \
        if (!InstantIoT::decodePad(s.value.text(), e.button, e.kind)) return;    \
        e.widgetId   = "";                                                 \
        e.buttonName = InstantIoT::padName(e.button);                      \
        FN(e);                                                             \
    }                                                                      \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP, /* geste */ true);                   \
    static void FN(const InstantIoT::DirectionPadEvent& e)

/** `IEmergencyButton(I7) { WHEN_TRIGGERED … }` */
#define IEmergencyButton(ref) \
    _IIO_IEMB(ref, _IIO_UID(_iioEmF_), _IIO_UID(_iioEmT_), _IIO_UID(_iioEmR_))
#define _IIO_IEMB(ref, FN, TRAMP, REG)                                     \
    static void FN(const InstantIoT::EmergencyButtonEvent&);               \
    static void TRAMP(const InstantIoT::SignalEvent& s) {                  \
        InstantIoT::EmergencyButtonEvent e{};                              \
        e.widgetId = "";                                                   \
        e.kind = ((float)s.value >= 0.5f)                                  \
            ? InstantIoT::EmergencyEventKind::Trigger                      \
            : InstantIoT::EmergencyEventKind::Reset;                       \
        FN(e);                                                             \
    }                                                                      \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP, /* geste */ true);                   \
    static void FN(const InstantIoT::EmergencyButtonEvent& e)

// ============================================================
// 📶 ISignal(ref, decl) — declares a handler for a signal ADDRESS
//
//   ISignal(I5, float target) { setpoint = target; };
//   ISignal(I6, bool on)      { digitalWrite(PUMP, on); };
//   ISignal(I7, const char* mode) { applyMode(mode); };
//
// Keyed on `I0`..`I255` rather than on a string, so dispatch is
// a byte compare and not a strcmp.
//
// There is no `WHEN_` guard here, unlike the widget blocks, and
// the difference is not an oversight. A button sends several
// KINDS of event to the same id — press, release, toggle — so its
// block has to sort them. A signal has exactly one thing that can
// happen to it: it was written. A `WHEN_WRITTEN` would announce a
// choice that does not exist.
//
// The declaration is written whole because the board is the only
// place that knows what the address holds. The block becomes an
// ordinary function taking that type; a trampoline converts the
// value on the way in.
// ============================================================

#define ISignal(ref, decl) \
    _IIO_ISIGNAL_IMPL(ref, decl, _IIO_UID(_iioSigF_), _IIO_UID(_iioSigT_), _IIO_UID(_iioSigR_))
#define _IIO_ISIGNAL_IMPL(ref, decl, FN, TRAMP, REG)                       \
    static void FN(decl);                                                  \
    static void TRAMP(const InstantIoT::SignalEvent& e) { FN(e.value); }   \
    static InstantIoT::SignalRegistrar REG(ref, &TRAMP);                   \
    static void FN(decl)

// ============================================================
// 🧩 Per-type predicates (overloaded) — enable a single
// `WHEN_RELEASED` that works for Button and Joystick
// ============================================================

namespace InstantIoT {

inline bool _whenPressed(const SimpleButtonEvent& e)       { return e.kind == ButtonEventKind::Press; }
inline bool _whenPressed(const AdvancedButtonEvent& e)     { return e.kind == ButtonEventKind::Press; }

inline bool _whenReleased(const SimpleButtonEvent& e)      { return e.kind == ButtonEventKind::Release; }
inline bool _whenReleased(const AdvancedButtonEvent& e)    { return e.kind == ButtonEventKind::Release; }
inline bool _whenReleased(const JoystickEvent& e)          { return e.kind == JoystickEventKind::Released; }

inline bool _whenLongPressed(const SimpleButtonEvent& e)   { return e.kind == ButtonEventKind::LongPress; }
inline bool _whenLongPressed(const AdvancedButtonEvent& e) { return e.kind == ButtonEventKind::LongPress; }

inline bool _whenToggled(const SimpleButtonEvent& e)       { return e.kind == ButtonEventKind::Toggle; }
inline bool _whenToggled(const AdvancedButtonEvent& e)     { return e.kind == ButtonEventKind::Toggle; }
inline bool _whenToggled(const SwitchEvent& e)             { return e.kind == SwitchEventKind::Toggle; }

inline bool _whenMoved(const JoystickEvent& e)             { return e.kind == JoystickEventKind::PositionChanged; }

// ── Switch ───────────────────────────────────────────────
inline bool _whenTurnedOn(const SwitchEvent& e)            { return e.kind == SwitchEventKind::TurnOn; }
inline bool _whenTurnedOff(const SwitchEvent& e)           { return e.kind == SwitchEventKind::TurnOff; }
inline bool _whenSwitchSetValue(const SwitchEvent& e)      { return e.kind == SwitchEventKind::SetValue; }

// ── Slider ───────────────────────────────────────────────
inline bool _whenChanging(const HorizontalSliderEvent& e)  { return e.kind == SliderEventKind::ValueChanging; }
inline bool _whenChanging(const VerticalSliderEvent& e)    { return e.kind == SliderEventKind::ValueChanging; }

inline bool _whenChanged(const HorizontalSliderEvent& e)   { return e.kind == SliderEventKind::ValueChanged; }
inline bool _whenChanged(const VerticalSliderEvent& e)     { return e.kind == SliderEventKind::ValueChanged; }

// ── DirectionPad ─────────────────────────────────────────
inline bool _whenPadPressed(const DirectionPadEvent& e)    { return e.kind == DPadEventKind::Press; }
inline bool _whenPadReleased(const DirectionPadEvent& e)   { return e.kind == DPadEventKind::Release; }
inline bool _whenPadLongPressed(const DirectionPadEvent& e){ return e.kind == DPadEventKind::LongPress; }

// ── SegmentedSwitch ──────────────────────────────────────
inline bool _whenSelectionChanged(const SegmentedSwitchEvent& e) { return e.kind == SegmentedEventKind::SelectionChanged; }
inline bool _whenSegmentSelected(const SegmentedSwitchEvent& e)  { return e.kind == SegmentedEventKind::SegmentSelected; }
inline bool _whenSegmentDeselected(const SegmentedSwitchEvent& e){ return e.kind == SegmentedEventKind::SegmentDeselected; }

// ── EmergencyButton ──────────────────────────────────────
inline bool _whenTriggered(const EmergencyButtonEvent& e)  { return e.kind == EmergencyEventKind::Trigger; }
inline bool _whenReset(const EmergencyButtonEvent& e)      { return e.kind == EmergencyEventKind::Reset; }

} // namespace InstantIoT

// ============================================================
// 🎯 WHEN_* — internal guards inside I<Widget> blocks
//
// `e` is the argument of the function generated by I<Widget>(id).
// Predicates are resolved by overload based on the type of `e`.
// ============================================================

// --- Without capture ----------------------------------------
#define WHEN_PRESSED      if (InstantIoT::_whenPressed(e))
#define WHEN_RELEASED     if (InstantIoT::_whenReleased(e))
#define WHEN_LONG_PRESSED if (InstantIoT::_whenLongPressed(e))

// --- With capture of a bool variable ------------------------
// "if init-statement" idiom (C++17) to have only one if
#define WHEN_TOGGLED(var_isOn)                                                 \
    if (InstantIoT::_whenToggled(e))                                           \
        if (bool var_isOn = e.isOn; true)

// --- With capture of two float variables (joystick) --------
// Two nested if-init (C++17) — each condition is a `true` bool,
// so the user block always runs once if the kind matches.
#define WHEN_MOVED(var_x, var_y)                                               \
    if (InstantIoT::_whenMoved(e))                                             \
        if (float var_x = e.x; true)                                           \
            if (float var_y = e.y; true)

// ============================================================
// 🎚️ SLIDER  (H and V — same DSL)
// ============================================================
// Streaming during drag (ValueChanging) — captures the float value
#define WHEN_CHANGING(var_v)                                                   \
    if (InstantIoT::_whenChanging(e))                                          \
        if (float var_v = e.value; true)

// Final value on release (ValueChanged) — captures the float value
#define WHEN_CHANGED(var_v)                                                    \
    if (InstantIoT::_whenChanged(e))                                           \
        if (float var_v = e.value; true)

// ============================================================
// 🔘 SWITCH
// ============================================================
#define WHEN_TURNED_ON     if (InstantIoT::_whenTurnedOn(e))
#define WHEN_TURNED_OFF    if (InstantIoT::_whenTurnedOff(e))
// WHEN_TOGGLED(isOn) is already defined (overload on SwitchEvent).
// WHEN_SWITCH_SET(isOn) captures e.isOn for events with kind=SetValue.
#define WHEN_SWITCH_SET(var_isOn)                                              \
    if (InstantIoT::_whenSwitchSetValue(e))                                    \
        if (bool var_isOn = e.isOn; true)

// ============================================================
// 🎮 DIRECTION PAD
// ============================================================
// Captures e.button (DPadButton enum) — user inspects with a switch
// ── La croix, touche par touche ──────────────────────────────
//
// `WHEN_PAD_PRESSED(btn)` rend la touche dans une variable, et on se retrouve
// à écrire un `switch` dedans — l'escalier de `if` déplacé d'un cran. Les
// formes nommées évitent ça quand les touches font des choses différentes ;
// la forme à variable reste pour quand elles font la même avec un paramètre.
//
// Les cinq de la croix sont nommées. Les huit autres — A, B, X, Y et les
// quatre formes — passent par la forme à variable : les nommer ferait
// trente-neuf macros pour des croquis qui les traitent en boucle.
#define _IIO_PAD(T, K)                                                         \
    if (e.button == InstantIoT::DPadButton::T &&                               \
        e.kind   == InstantIoT::DPadEventKind::K)

#define WHEN_UP              _IIO_PAD(Up, Press)
#define WHEN_DOWN            _IIO_PAD(Down, Press)
#define WHEN_LEFT            _IIO_PAD(Left, Press)
#define WHEN_RIGHT           _IIO_PAD(Right, Press)
#define WHEN_CENTER          _IIO_PAD(Center, Press)

#define WHEN_UP_LONG         _IIO_PAD(Up, LongPress)
#define WHEN_DOWN_LONG       _IIO_PAD(Down, LongPress)
#define WHEN_LEFT_LONG       _IIO_PAD(Left, LongPress)
#define WHEN_RIGHT_LONG      _IIO_PAD(Right, LongPress)
#define WHEN_CENTER_LONG     _IIO_PAD(Center, LongPress)

#define WHEN_UP_RELEASE      _IIO_PAD(Up, Release)
#define WHEN_DOWN_RELEASE    _IIO_PAD(Down, Release)
#define WHEN_LEFT_RELEASE    _IIO_PAD(Left, Release)
#define WHEN_RIGHT_RELEASE   _IIO_PAD(Right, Release)
#define WHEN_CENTER_RELEASE  _IIO_PAD(Center, Release)

/** N'importe quelle touche relâchée — savoir laquelle n'intéresse presque jamais. */
#define WHEN_RELEASED_ANY    if (e.kind == InstantIoT::DPadEventKind::Release)

#define WHEN_PAD_PRESSED(var_btn)                                              \
    if (InstantIoT::_whenPadPressed(e))                                        \
        if (InstantIoT::DPadButton var_btn = e.button; true)

#define WHEN_PAD_RELEASED(var_btn)                                             \
    if (InstantIoT::_whenPadReleased(e))                                       \
        if (InstantIoT::DPadButton var_btn = e.button; true)

#define WHEN_PAD_LONG_PRESSED(var_btn)                                         \
    if (InstantIoT::_whenPadLongPressed(e))                                    \
        if (InstantIoT::DPadButton var_btn = e.button; true)

// ============================================================
// 🔀 SEGMENTED SWITCH
// ============================================================
// Captures selectedIndex (int)
#define WHEN_SELECTION_CHANGED(var_idx)                                        \
    if (InstantIoT::_whenSelectionChanged(e))                                  \
        if (int var_idx = e.selectedIndex; true)

#define WHEN_SEGMENT_SELECTED(var_idx)                                         \
    if (InstantIoT::_whenSegmentSelected(e))                                   \
        if (int var_idx = e.selectedIndex; true)

#define WHEN_SEGMENT_DESELECTED(var_idx)                                       \
    if (InstantIoT::_whenSegmentDeselected(e))                                 \
        if (int var_idx = e.selectedIndex; true)

// ============================================================
// 🚨 EMERGENCY BUTTON
// ============================================================
#define WHEN_TRIGGERED  if (InstantIoT::_whenTriggered(e))
#define WHEN_RESET      if (InstantIoT::_whenReset(e))
