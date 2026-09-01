#pragma once
/**
 * ============================================================
 *  InstantIoTWhen.hpp — one block per ADDRESS
 * ============================================================
 *
 * A block aims at an address — `I0`, `I2` — not at a widget name. The app
 * no longer sends widget events: it writes a SIGNAL, and these blocks
 * decode it.
 *
 * ## The shape rule
 *
 * **The head carries the data, `WHEN_` carries the kind.**
 *
 * One kind of event — a position, a value — and the parameters are enough:
 *
 *   IJoystick(I2, float x, float y) { driveMotors(x, y); }
 *   IHorizontalSlider(I1, float v)  { analogWrite(LED_PIN, v); }
 *   ISwitch(I3, bool on)            { digitalWrite(RELAY_PIN, on); }
 *   ISegmentedSwitch(I4, int index) { mode = index; }
 *
 * Several kinds, and `WHEN_` sorts them — which avoids the staircase of
 * `if`:
 *
 *   ISimpleButton(I0) {
 *       WHEN_PRESSED      { Serial.println("press"); }
 *       WHEN_RELEASED     { Serial.println("release"); }
 *       WHEN_LONG_PRESSED { Serial.println("hold"); }
 *   };
 *
 *   IDirectionPad(I5) {
 *       WHEN_UP      { forward(); }
 *       WHEN_UP_LONG { faster(); }
 *       WHEN_PAD_PRESSED(btn) { motor(btn); }   // all five at once
 *   };
 *
 * ## What the board reads
 *
 * A button writes a VALUE, and the gesture is in it by convention:
 * **1 press, 0 release, 2 long press**. A joystick writes `"0.42,-0.15"`,
 * a direction pad writes `"UP"`, `"UP_LONG"`, `"UP_RELEASE"`. The decoding
 * lives in `SignalToWidget.hpp`, written once rather than in every sketch.
 *
 * ## Several blocks, one address
 *
 * Blocks of different kinds may aim at the same address — an
 * `ISignal(I0, float v)` next to an `ISimpleButton(I0)`. Both are called,
 * except on a RESTORE: on reconnect the server sends back the last value,
 * and only `ISignal` receives it. A state is restored, a gesture is not
 * replayed.
 *
 * Each block costs one linked-list node, with no allocation.
 *
 * ## The trailing semicolon
 *
 * A block closes with `};` — the `;` ends the definition of the registrar
 * the macro generates.
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
// 🎯 I<Widget>(Ix) — one block, at one ADDRESS
//
// These blocks listened on a channel nobody feeds any more: they were
// registered by NAME, on widget events the app no longer sends. It writes
// a SIGNAL, at an address, and the content is a number or text. So they
// listen to that, and decode it — that is all that changes.
//
// The vocabulary does not move: `WHEN_PRESSED` receives what it used to,
// and a sketch only changes `"btn1"` into `I0`.
//
// **The head carries the data, `WHEN_` carries the kind.** One kind of
// event — a position, a value — and the parameters are enough. Several
// kinds, and `WHEN_` sorts them, which avoids the staircase of `if`.
// ============================================================

// ── One kind: the data in the head ───────────────────────────

/** `IJoystick(I2, float x, float y) { … }` — the app writes `"0.42,-0.15"`. */
#define IJoystick(ref, ...) \
    _IIO_IJOY(ref, _IIO_UID(_iioJyF_), _IIO_UID(_iioJyT_), _IIO_UID(_iioJyR_), __VA_ARGS__)
#define _IIO_IJOY(ref, FN, TRAMP, REG, ...)                                \
    static void FN(__VA_ARGS__);                                           \
    static void TRAMP(const iiot::SignalEvent& e) {                  \
        float _iioX = 0.0f, _iioY = 0.0f;                                  \
        if (iiot::decodePosition(e.value.text(), _iioX, _iioY))            \
            FN(_iioX, _iioY);                                              \
    }                                                                      \
    static iiot::SignalRegistrar REG(ref, &TRAMP, /* gesture */ true);                   \
    static void FN(__VA_ARGS__)

/** `IHorizontalSlider(I4, float v) { … }` */
#define IHorizontalSlider(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioHsF_), _IIO_UID(_iioHsT_), _IIO_UID(_iioHsR_), __VA_ARGS__)
/** `IVerticalSlider(I5, float v) { … }` */
#define IVerticalSlider(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioVsF_), _IIO_UID(_iioVsT_), _IIO_UID(_iioVsR_), __VA_ARGS__)
/** `ISwitch(I1, bool on) { … }` — the parameter type does the conversion. */
#define ISwitch(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioSwF_), _IIO_UID(_iioSwT_), _IIO_UID(_iioSwR_), __VA_ARGS__)
/** `ISegmentedSwitch(I6, int index) { … }` */
#define ISegmentedSwitch(ref, ...) \
    _IIO_INUM(ref, _IIO_UID(_iioSgF_), _IIO_UID(_iioSgT_), _IIO_UID(_iioSgR_), __VA_ARGS__)

// A value, returned in the type the block asks for. `SignalValue` knows how
// to convert to `float`, `int` or `bool` — the sketch's declaration chooses,
// not the frame.
#define _IIO_INUM(ref, FN, TRAMP, REG, ...)                                \
    static void FN(__VA_ARGS__);                                           \
    static void TRAMP(const iiot::SignalEvent& e) { FN(e.value); }   \
    static iiot::SignalRegistrar REG(ref, &TRAMP, /* gesture */ true);                   \
    static void FN(__VA_ARGS__)

// ── Several kinds: one block, and `WHEN_` to sort them ───────

/** `ISimpleButton(I0) { WHEN_PRESSED … }` — 1 press, 0 release, 2 long. */
#define ISimpleButton(ref) \
    _IIO_IBTN(ref, SimpleButtonEvent, _IIO_UID(_iioSbF_), _IIO_UID(_iioSbT_), _IIO_UID(_iioSbR_))
/** `IAdvancedButton(I0) { WHEN_PRESSED … }` */
#define IAdvancedButton(ref) \
    _IIO_IBTN(ref, AdvancedButtonEvent, _IIO_UID(_iioAbF_), _IIO_UID(_iioAbT_), _IIO_UID(_iioAbR_))

#define _IIO_IBTN(ref, EVT, FN, TRAMP, REG)                                \
    static void FN(const iiot::EVT&);                                \
    static void TRAMP(const iiot::SignalEvent& s) {                  \
        iiot::EVT e{};                                               \
        e.widgetId = "";                                                   \
        e.kind = iiot::gestureFromValue((float)s.value);             \
        e.isOn = (e.kind == iiot::ButtonEventKind::Press);           \
        FN(e);                                                             \
    }                                                                      \
    static iiot::SignalRegistrar REG(ref, &TRAMP, /* gesture */ true);                   \
    static void FN(const iiot::EVT& e)

/** `IDirectionPad(I3) { WHEN_UP … }` — the app writes `"UP"`, `"UP_LONG"`… */
#define IDirectionPad(ref) \
    _IIO_IDP(ref, _IIO_UID(_iioDpF_), _IIO_UID(_iioDpT_), _IIO_UID(_iioDpR_))
#define _IIO_IDP(ref, FN, TRAMP, REG)                                      \
    static void FN(const iiot::DirectionPadEvent&);                  \
    static void TRAMP(const iiot::SignalEvent& s) {                  \
        iiot::DirectionPadEvent e{};                                 \
        if (!iiot::decodePad(s.value.text(), e.button, e.kind)) return;    \
        e.widgetId   = "";                                                 \
        e.buttonName = iiot::padName(e.button);                      \
        FN(e);                                                             \
    }                                                                      \
    static iiot::SignalRegistrar REG(ref, &TRAMP, /* gesture */ true);                   \
    static void FN(const iiot::DirectionPadEvent& e)

/** `IEmergencyButton(I7) { WHEN_TRIGGERED … }` */
#define IEmergencyButton(ref) \
    _IIO_IEMB(ref, _IIO_UID(_iioEmF_), _IIO_UID(_iioEmT_), _IIO_UID(_iioEmR_))
#define _IIO_IEMB(ref, FN, TRAMP, REG)                                     \
    static void FN(const iiot::EmergencyButtonEvent&);               \
    static void TRAMP(const iiot::SignalEvent& s) {                  \
        iiot::EmergencyButtonEvent e{};                              \
        e.widgetId = "";                                                   \
        e.kind = ((float)s.value >= 0.5f)                                  \
            ? iiot::EmergencyEventKind::Trigger                      \
            : iiot::EmergencyEventKind::Reset;                       \
        FN(e);                                                             \
    }                                                                      \
    static iiot::SignalRegistrar REG(ref, &TRAMP, /* gesture */ true);                   \
    static void FN(const iiot::EmergencyButtonEvent& e)

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
    static void TRAMP(const iiot::SignalEvent& e) { FN(e.value); }   \
    static iiot::SignalRegistrar REG(ref, &TRAMP);                   \
    static void FN(decl)

// ============================================================
// 🧩 Per-type predicates (overloaded) — enable a single
// `WHEN_RELEASED` that works for Button and Joystick
// ============================================================

namespace iiot {

inline bool _whenPressed(const SimpleButtonEvent& e)       { return e.kind == ButtonEventKind::Press; }
inline bool _whenPressed(const AdvancedButtonEvent& e)     { return e.kind == ButtonEventKind::Press; }

inline bool _whenReleased(const SimpleButtonEvent& e)      { return e.kind == ButtonEventKind::Release; }
inline bool _whenReleased(const AdvancedButtonEvent& e)    { return e.kind == ButtonEventKind::Release; }

inline bool _whenLongPressed(const SimpleButtonEvent& e)   { return e.kind == ButtonEventKind::LongPress; }
inline bool _whenLongPressed(const AdvancedButtonEvent& e) { return e.kind == ButtonEventKind::LongPress; }


// ── DirectionPad ─────────────────────────────────────────
inline bool _whenPadPressed(const DirectionPadEvent& e)    { return e.kind == DPadEventKind::Press; }
inline bool _whenPadReleased(const DirectionPadEvent& e)   { return e.kind == DPadEventKind::Release; }
inline bool _whenPadLongPressed(const DirectionPadEvent& e){ return e.kind == DPadEventKind::LongPress; }

// ── EmergencyButton ──────────────────────────────────────
inline bool _whenTriggered(const EmergencyButtonEvent& e)  { return e.kind == EmergencyEventKind::Trigger; }
inline bool _whenReset(const EmergencyButtonEvent& e)      { return e.kind == EmergencyEventKind::Reset; }

} // namespace iiot

// ============================================================
// 🎯 WHEN_* — internal guards inside I<Widget> blocks
//
// `e` is the argument of the function generated by I<Widget>(id).
// Predicates are resolved by overload based on the type of `e`.
// ============================================================

#define WHEN_PRESSED      if (iiot::_whenPressed(e))
#define WHEN_RELEASED     if (iiot::_whenReleased(e))
#define WHEN_LONG_PRESSED if (iiot::_whenLongPressed(e))

// ============================================================
// 🎮 DIRECTION PAD
// ============================================================
// Captures e.button (DPadButton enum) — user inspects with a switch
// ── The pad, key by key ──────────────────────────────────────
//
// `WHEN_PAD_PRESSED(btn)` hands the key back in a variable, and you end up
// writing a `switch` inside it — the staircase of `if` moved one floor
// down. The named forms avoid that when the keys do different things; the
// variable form remains for when they do the same thing with a parameter.
//
// The five pad keys are named. The other eight — A, B, X, Y and the four
// shapes — go through the variable form: naming them would make
// thirty-nine macros for sketches that handle them in a loop.
#define _IIO_PAD(T, K)                                                         \
    if (e.button == iiot::DPadButton::T &&                               \
        e.kind   == iiot::DPadEventKind::K)

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

/** Any key released — which one is almost never interesting. */
#define WHEN_RELEASED_ANY    if (e.kind == iiot::DPadEventKind::Release)

#define WHEN_PAD_PRESSED(var_btn)                                              \
    if (iiot::_whenPadPressed(e))                                        \
        if (iiot::DPadButton var_btn = e.button; true)

#define WHEN_PAD_RELEASED(var_btn)                                             \
    if (iiot::_whenPadReleased(e))                                       \
        if (iiot::DPadButton var_btn = e.button; true)

#define WHEN_PAD_LONG_PRESSED(var_btn)                                         \
    if (iiot::_whenPadLongPressed(e))                                    \
        if (iiot::DPadButton var_btn = e.button; true)

// ============================================================
// 🚨 EMERGENCY BUTTON
// ============================================================
#define WHEN_TRIGGERED  if (iiot::_whenTriggered(e))
#define WHEN_RESET      if (iiot::_whenReset(e))
