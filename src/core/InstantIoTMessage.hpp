#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * InstantIoTMessage.hpp - Structures de messages
 *************************************************************/

#include <Arduino.h>
#include "../InstantIoTConfig.h"

namespace InstantIoT {

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

// ============================================================
// 🎚️ SLIDER EVENTS
// ============================================================

enum class SliderEventKind : uint8_t {
    ValueChanging = 0,
    ValueChanged,
    DragStarted,
    DragEnded
};

struct HorizontalSliderEvent {
    const char* widgetId;
    SliderEventKind kind;
    float value;
    float startValue;
    float finalValue;
    
    bool isValueChanging() const { return kind == SliderEventKind::ValueChanging; }
    bool isValueChanged() const { return kind == SliderEventKind::ValueChanged; }
    bool isDragStarted() const { return kind == SliderEventKind::DragStarted; }
    bool isDragEnded() const { return kind == SliderEventKind::DragEnded; }
};

struct VerticalSliderEvent {
    const char* widgetId;
    SliderEventKind kind;
    float value;
    float startValue;
    float finalValue;
    
    bool isValueChanging() const { return kind == SliderEventKind::ValueChanging; }
    bool isValueChanged() const { return kind == SliderEventKind::ValueChanged; }
    bool isDragStarted() const { return kind == SliderEventKind::DragStarted; }
    bool isDragEnded() const { return kind == SliderEventKind::DragEnded; }
};

// ============================================================
// 🔘 SWITCH EVENTS
// ============================================================

enum class SwitchEventKind : uint8_t {
    TurnOn = 0,
    TurnOff,
    Toggle,
    SetValue
};

struct SwitchEvent {
    const char* widgetId;
    SwitchEventKind kind;
    bool isOn;
    
    bool isTurnOn() const { return kind == SwitchEventKind::TurnOn; }
    bool isTurnOff() const { return kind == SwitchEventKind::TurnOff; }
    bool isToggle() const { return kind == SwitchEventKind::Toggle; }
    bool isSetValue() const { return kind == SwitchEventKind::SetValue; }
};

// ============================================================
// 🕹️ JOYSTICK EVENTS
// ============================================================

enum class JoystickEventKind : uint8_t {
    PositionChanged = 0,
    Released
};

struct JoystickEvent {
    const char* widgetId;
    JoystickEventKind kind;
    float x;    // -1.0 à 1.0
    float y;    // -1.0 à 1.0
    
    bool isPositionChanged() const { return kind == JoystickEventKind::PositionChanged; }
    bool isReleased() const { return kind == JoystickEventKind::Released; }
    
    float magnitude() const { return sqrt(x * x + y * y); }
    float angle() const { return atan2(y, x) * 180.0f / PI; }
};

// ============================================================
// 🎮 DIRECTION PAD EVENTS
// ============================================================

enum class DPadButton : uint8_t {
    Up = 0,
    Down,
    Left,
    Right,
    Center,
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

// ============================================================
// 🔀 SEGMENTED SWITCH EVENTS
// ============================================================

enum class SegmentedEventKind : uint8_t {
    SelectionChanged = 0,
    SegmentSelected,
    SegmentDeselected
};

struct SegmentedSwitchEvent {
    const char* widgetId;
    SegmentedEventKind kind;
    int selectedIndex;
    const char* segmentId;      // ID du segment concerné
    const char* selectedIds;    // IDs sélectionnés (multi-select)
    int count;
    
    bool isSelectionChanged() const { return kind == SegmentedEventKind::SelectionChanged; }
    bool isSegmentSelected() const { return kind == SegmentedEventKind::SegmentSelected; }
    bool isSegmentDeselected() const { return kind == SegmentedEventKind::SegmentDeselected; }
};

// ============================================================
// 📊 GENERIC WIDGET REQUEST
// ============================================================

struct WidgetRequest {
    const char* widgetId;
    const char* requestType;
};

} // namespace InstantIoT

// ============================================================
// 🌍 EXPOSE GLOBAL
// ============================================================

using SimpleButtonEvent = InstantIoT::SimpleButtonEvent;
using AdvancedButtonEvent = InstantIoT::AdvancedButtonEvent;
using EmergencyButtonEvent = InstantIoT::EmergencyButtonEvent;
using HorizontalSliderEvent = InstantIoT::HorizontalSliderEvent;
using VerticalSliderEvent = InstantIoT::VerticalSliderEvent;
using SwitchEvent = InstantIoT::SwitchEvent;
using JoystickEvent = InstantIoT::JoystickEvent;
using DirectionPadEvent = InstantIoT::DirectionPadEvent;
using SegmentedSwitchEvent = InstantIoT::SegmentedSwitchEvent;
using WidgetRequest = InstantIoT::WidgetRequest;

using ButtonEventKind = InstantIoT::ButtonEventKind;
using EmergencyEventKind = InstantIoT::EmergencyEventKind;
using SliderEventKind = InstantIoT::SliderEventKind;
using SwitchEventKind = InstantIoT::SwitchEventKind;
using JoystickEventKind = InstantIoT::JoystickEventKind;
using DPadButton = InstantIoT::DPadButton;
using DPadEventKind = InstantIoT::DPadEventKind;
using SegmentedEventKind = InstantIoT::SegmentedEventKind;
