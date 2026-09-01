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
 * Ce que la 2.0 n'a plus a representer.
 *
 * Les structures des curseurs, de l'interrupteur, de la manette et du choix
 * segmente vivaient ici. Plus rien ne les construisait : leurs blocs portent
 * la valeur dans leur TETE — `ISwitch(I3, bool on)` — et il n'y a plus
 * d'evenement a emballer.
 *
 * Ce qui reste ci-dessous a un producteur : `SignalToWidget` fabrique un
 * geste de bouton depuis une valeur (1, 0, 2) et une touche de croix depuis
 * un mot ("UP", "UP_LONG").
 */

/**
 * Les touches d'une croix, telles que l'app les nomme.
 *
 * Elle en propose treize — la croix, les quatre lettres d'une manette, les
 * quatre formes d'une autre. La lib n'en connaissait que cinq : une croix
 * reglee en A/B/X/Y envoyait des mots qu'elle rangeait tous dans `Unknown`,
 * et le croquis ne pouvait pas les distinguer.
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
