#pragma once
/**
 * ============================================================
 *  InstantIoTWhen.hpp — Macros déclaratives par-widget
 * ============================================================
 *
 * Syntaxe raccourcie pour écrire des handlers par widget id :
 *
 *   ISimpleButton("btn1") {
 *       WHEN_TOGGLED(isOn) { analogWrite(LED_PIN, isOn ? 255 : 0); }
 *       WHEN_PRESSED       { Serial.println("press"); }
 *       WHEN_LONG_PRESSED  { Serial.println("hold"); }
 *       WHEN_RELEASED      { Serial.println("release"); }
 *   };
 *
 *   IAdvancedButton("action1") {
 *       WHEN_TOGGLED(on) { digitalWrite(RELAY_PIN, on); }
 *   };
 *
 *   IJoystick("joy1") {
 *       WHEN_MOVED(x, y) { driveMotors((int)(y * 2.55f), (int)(x * 2.55f)); }
 *       WHEN_RELEASED    { driveMotors(0, 0); }
 *   };
 *
 * ## Multi-instance
 *
 * Plusieurs blocs du même widget type avec des ids distincts
 * coexistent :
 *
 *   ISimpleButton("btn1") { ... };
 *   ISimpleButton("btn2") { ... };
 *
 * Chaque bloc enregistre son propre handler dans une linked list
 * intrusive (12 B RAM par bloc sur ESP32). Dispatch : 1 strcmp
 * par handler enregistré pour le type concerné.
 *
 * ## Coexistence legacy
 *
 * Les `void onSimpleButtonEvent(const SimpleButtonEvent&)` etc.
 * continuent de fonctionner : les deux chemins sont appelés. Tu peux
 * migrer sketch par sketch sans casse.
 *
 * ## ; final obligatoire
 *
 * Le bloc se ferme sur `};` — le `;` termine la définition
 * `static auto _reg = WidgetRegistrar(...)` que la macro génère.
 *
 * ============================================================
 */

#include <Arduino.h>
#include "../core/Registry.hpp"
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
// 🎯 I<Widget>(id) — déclare un handler scope-fichier pour l'id
// ============================================================

#define ISimpleButton(id) _IIO_ISIMPLEBTN_IMPL(id, _IIO_UID(_iioSbF_), _IIO_UID(_iioSbR_))
#define _IIO_ISIMPLEBTN_IMPL(id, FN, REG)                                  \
    static void FN(const InstantIoT::SimpleButtonEvent&);                  \
    static InstantIoT::WidgetRegistrar<InstantIoT::SimpleButtonEvent>      \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::SimpleButtonEvent& e)

#define IAdvancedButton(id) _IIO_IADVBTN_IMPL(id, _IIO_UID(_iioAbF_), _IIO_UID(_iioAbR_))
#define _IIO_IADVBTN_IMPL(id, FN, REG)                                     \
    static void FN(const InstantIoT::AdvancedButtonEvent&);                \
    static InstantIoT::WidgetRegistrar<InstantIoT::AdvancedButtonEvent>    \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::AdvancedButtonEvent& e)

#define IJoystick(id) _IIO_IJOY_IMPL(id, _IIO_UID(_iioJyF_), _IIO_UID(_iioJyR_))
#define _IIO_IJOY_IMPL(id, FN, REG)                                        \
    static void FN(const InstantIoT::JoystickEvent&);                      \
    static InstantIoT::WidgetRegistrar<InstantIoT::JoystickEvent>          \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::JoystickEvent& e)

#define IHorizontalSlider(id) _IIO_IHSL_IMPL(id, _IIO_UID(_iioHsF_), _IIO_UID(_iioHsR_))
#define _IIO_IHSL_IMPL(id, FN, REG)                                        \
    static void FN(const InstantIoT::HorizontalSliderEvent&);              \
    static InstantIoT::WidgetRegistrar<InstantIoT::HorizontalSliderEvent>  \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::HorizontalSliderEvent& e)

#define IVerticalSlider(id) _IIO_IVSL_IMPL(id, _IIO_UID(_iioVsF_), _IIO_UID(_iioVsR_))
#define _IIO_IVSL_IMPL(id, FN, REG)                                        \
    static void FN(const InstantIoT::VerticalSliderEvent&);                \
    static InstantIoT::WidgetRegistrar<InstantIoT::VerticalSliderEvent>    \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::VerticalSliderEvent& e)

#define ISwitch(id) _IIO_ISW_IMPL(id, _IIO_UID(_iioSwF_), _IIO_UID(_iioSwR_))
#define _IIO_ISW_IMPL(id, FN, REG)                                         \
    static void FN(const InstantIoT::SwitchEvent&);                        \
    static InstantIoT::WidgetRegistrar<InstantIoT::SwitchEvent>            \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::SwitchEvent& e)

#define IDirectionPad(id) _IIO_IDP_IMPL(id, _IIO_UID(_iioDpF_), _IIO_UID(_iioDpR_))
#define _IIO_IDP_IMPL(id, FN, REG)                                         \
    static void FN(const InstantIoT::DirectionPadEvent&);                  \
    static InstantIoT::WidgetRegistrar<InstantIoT::DirectionPadEvent>      \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::DirectionPadEvent& e)

#define ISegmentedSwitch(id) _IIO_ISEG_IMPL(id, _IIO_UID(_iioSgF_), _IIO_UID(_iioSgR_))
#define _IIO_ISEG_IMPL(id, FN, REG)                                        \
    static void FN(const InstantIoT::SegmentedSwitchEvent&);               \
    static InstantIoT::WidgetRegistrar<InstantIoT::SegmentedSwitchEvent>   \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::SegmentedSwitchEvent& e)

#define IAdvancedChart(id) _IIO_ICH_IMPL(id, _IIO_UID(_iioChF_), _IIO_UID(_iioChR_))
#define _IIO_ICH_IMPL(id, FN, REG)                                         \
    static void FN(const InstantIoT::WidgetRequest&);                      \
    static InstantIoT::WidgetRegistrar<InstantIoT::WidgetRequest>          \
        REG(id, &FN);                                                      \
    static void FN(const InstantIoT::WidgetRequest& e)

// ============================================================
// 🧩 Prédicats par type (overloadés) — permettent un `WHEN_RELEASED`
// unique qui marche pour Button et Joystick
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

// ── AdvancedChart (WidgetRequest) ────────────────────────
inline bool _whenRequestData(const WidgetRequest& e)       { return e.requestType && strcmp(e.requestType, "requestdata") == 0; }
inline bool _whenRequestRefresh(const WidgetRequest& e)    { return e.requestType && strcmp(e.requestType, "requestrefresh") == 0; }

} // namespace InstantIoT

// ============================================================
// 🎯 WHEN_* — guards internes aux blocs I<Widget>
//
// `e` est l'argument de la fonction générée par I<Widget>(id).
// Les prédicats sont résolus par overload selon le type de `e`.
// ============================================================

// --- Sans capture -------------------------------------------
#define WHEN_PRESSED      if (InstantIoT::_whenPressed(e))
#define WHEN_RELEASED     if (InstantIoT::_whenReleased(e))
#define WHEN_LONG_PRESSED if (InstantIoT::_whenLongPressed(e))

// --- Avec capture d'une variable bool ----------------------
// idiom "if init-statement" (C++17) pour n'avoir qu'un seul if
#define WHEN_TOGGLED(var_isOn)                                                 \
    if (InstantIoT::_whenToggled(e))                                           \
        if (bool var_isOn = e.isOn; true)

// --- Avec capture de deux variables float (joystick) -------
// Deux if-init (C++17) imbriqués — chaque condition est un bool `true`,
// donc le bloc user s'exécute toujours une fois si le kind matche.
#define WHEN_MOVED(var_x, var_y)                                               \
    if (InstantIoT::_whenMoved(e))                                             \
        if (float var_x = e.x; true)                                           \
            if (float var_y = e.y; true)

// ============================================================
// 🎚️ SLIDER  (H et V — même DSL)
// ============================================================
// Streaming pendant drag (ValueChanging) — capture la valeur float
#define WHEN_CHANGING(var_v)                                                   \
    if (InstantIoT::_whenChanging(e))                                          \
        if (float var_v = e.value; true)

// Valeur finale au relâchement (ValueChanged) — capture la valeur float
#define WHEN_CHANGED(var_v)                                                    \
    if (InstantIoT::_whenChanged(e))                                           \
        if (float var_v = e.value; true)

// ============================================================
// 🔘 SWITCH
// ============================================================
#define WHEN_TURNED_ON     if (InstantIoT::_whenTurnedOn(e))
#define WHEN_TURNED_OFF    if (InstantIoT::_whenTurnedOff(e))
// WHEN_TOGGLED(isOn) est déjà défini (overload sur SwitchEvent).
// WHEN_SWITCH_SET(isOn) capture e.isOn pour les events kind=SetValue.
#define WHEN_SWITCH_SET(var_isOn)                                              \
    if (InstantIoT::_whenSwitchSetValue(e))                                    \
        if (bool var_isOn = e.isOn; true)

// ============================================================
// 🎮 DIRECTION PAD
// ============================================================
// Capture e.button (DPadButton enum) — scrute avec un switch à l'user
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
// Capture selectedIndex (int)
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
// 📊 ADVANCED CHART (WidgetRequest)
// ============================================================
#define WHEN_REQUEST_DATA    if (InstantIoT::_whenRequestData(e))
#define WHEN_REQUEST_REFRESH if (InstantIoT::_whenRequestRefresh(e))
