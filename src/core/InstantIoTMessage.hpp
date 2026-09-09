#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v1.1.0
 * 
 * InstantIoTMessage.hpp - Message structures
 *************************************************************/

#include <Arduino.h>
#include "../InstantIoTConfig.h"

namespace iiot {

// ============================================================
// 🔘 BUTTON EVENTS
// ============================================================

enum class ButtonEventKind : uint8_t {
    Press = 0,
    Release,
    LongPress,
    Toggle
};

struct SimpleButtonEvent {
    const char* widgetId;
    ButtonEventKind kind;
    bool isOn;
    
    bool isPress() const { return kind == ButtonEventKind::Press; }
    bool isRelease() const { return kind == ButtonEventKind::Release; }
    bool isLongPress() const { return kind == ButtonEventKind::LongPress; }
    bool isToggle() const { return kind == ButtonEventKind::Toggle; }
};

struct AdvancedButtonEvent {
    const char* widgetId;
    ButtonEventKind kind;
    bool isOn;
    
    bool isPress() const { return kind == ButtonEventKind::Press; }
    bool isRelease() const { return kind == ButtonEventKind::Release; }
    bool isLongPress() const { return kind == ButtonEventKind::LongPress; }
    bool isToggle() const { return kind == ButtonEventKind::Toggle; }
};

// ============================================================
// 🚨 EMERGENCY BUTTON EVENTS
// ============================================================

enum class EmergencyEventKind : uint8_t {
    Trigger = 0,
    Reset
};

struct EmergencyButtonEvent {
    const char* widgetId;
    EmergencyEventKind kind;
    
    bool isTrigger() const { return kind == EmergencyEventKind::Trigger; }
    bool isReset() const { return kind == EmergencyEventKind::Reset; }
};

/**
 * What 2.0 no longer has to represent.
 *
 * The slider, switch, joystick and segmented-switch structs used to live
 * here. Nothing built them any more: their blocks carry the value in their
 * HEAD — `ISwitch(I3, bool on)` — and there is no event left to wrap.
 *
 * What remains below has a producer: `SignalToWidget` makes a button
 * gesture out of a value (1, 0, 2) and a pad key out of a word ("UP",
 * "UP_LONG").
 */

/**
 * The keys of a direction pad, as the app names them.
 *
 * It offers thirteen — the cross, the four letters of one gamepad, the
 * four shapes of another. The library only knew five: a pad set to
 * A/B/X/Y sent words it filed all under `Unknown`, and the sketch could
 * not tell them apart.
 */
enum class DPadButton : uint8_t {
    Up = 0,
    Down,
    Left,
    Right,
    Center,
    A,
    B,
    X,
    Y,
    Triangle,
    Circle,
    Square,
    Cross,
    Unknown
};

enum class DPadEventKind : uint8_t {
    Press = 0,
    Release,
    LongPress
};

struct DirectionPadEvent {
    const char* widgetId;
    DPadButton button;
    DPadEventKind kind;
    const char* buttonName;  // "up", "down", "left", "right", "center"
    
    bool isPress() const { return kind == DPadEventKind::Press; }
    bool isRelease() const { return kind == DPadEventKind::Release; }
    bool isLongPress() const { return kind == DPadEventKind::LongPress; }
    
    bool isUp() const { return button == DPadButton::Up; }
    bool isDown() const { return button == DPadButton::Down; }
    bool isLeft() const { return button == DPadButton::Left; }
    bool isRight() const { return button == DPadButton::Right; }
    bool isCenter() const { return button == DPadButton::Center; }
};

} // namespace iiot

// ============================================================
// 🌍 EXPOSE GLOBAL
// ============================================================

using SimpleButtonEvent = iiot::SimpleButtonEvent;
using AdvancedButtonEvent = iiot::AdvancedButtonEvent;
using EmergencyButtonEvent = iiot::EmergencyButtonEvent;
using DirectionPadEvent = iiot::DirectionPadEvent;

using ButtonEventKind = iiot::ButtonEventKind;
using EmergencyEventKind = iiot::EmergencyEventKind;
using DPadButton = iiot::DPadButton;
using DPadEventKind = iiot::DPadEventKind;
