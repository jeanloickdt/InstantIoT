#pragma once

/*************************************************************
 * ⚡ InstantIoT Library v2.0
 * 
 * InstantIoTMacros.hpp — DSL fluide pour les callbacks
 * 
 * Usage:
 * 
 *   void onSimpleButtonEvent(const SimpleButtonEvent& e) {
 *       ON_PRESS("btn1") { digitalWrite(LED, HIGH); }
 *       ON_RELEASE("btn1") { digitalWrite(LED, LOW); }
 *       ON_LONG_PRESS("btn1") { resetSystem(); }
 *   }
 * 
 *   void onJoystickEvent(const JoystickEvent& e) {
 *       ON_JOYSTICK("joy1", x, y) { setMotors(x, y); }
 *       ON_JOYSTICK_RELEASED("joy1") { stopMotors(); }
 *   }
 * 
 *   void onHorizontalSliderEvent(const HorizontalSliderEvent& e) {
 *       ON_VALUE_CHANGED("slider1", value) {
 *           analogWrite(PWM_PIN, map(value, 0, 100, 0, 255));
 *       }
 *   }
 * 
 * Copyright (c) 2025 InstantIoT
 * MIT License
 *************************************************************/

#include <Arduino.h>
#include <string.h>

// ============================================================
// 🔍 HELPERS INTERNES
// ============================================================

#define _INSTANTIOT_IS_ID(e, id_str) (strcmp((e).widgetId, (id_str)) == 0)

// Comptage d'arguments (1, 2, ou 3)
#define _INSTANTIOT_ARG_COUNT(...) _INSTANTIOT_ARG_COUNT_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define _INSTANTIOT_ARG_COUNT_IMPL(_1, _2, _3, N, ...) N

#define _INSTANTIOT_CONCAT_IMPL(a, b) a##b
#define _INSTANTIOT_CONCAT(a, b) _INSTANTIOT_CONCAT_IMPL(a, b)

// ============================================================
// 🔘 SIMPLE BUTTON & ADVANCED BUTTON
// ============================================================

#define ON_PRESS(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::ButtonEventKind::Press)

#define ON_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::ButtonEventKind::Release)

#define ON_LONG_PRESS(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::ButtonEventKind::LongPress)

#define ON_TOGGLE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::ButtonEventKind::Toggle)

// Avec capture de isOn
#define ON_TOGGLE_STATE(id, var_isOn) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::ButtonEventKind::Toggle) \
        if (bool var_isOn = e.isOn; true)

// ============================================================
// 🚨 EMERGENCY BUTTON
// ============================================================

#define ON_TRIGGER(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::EmergencyEventKind::Trigger)

#define ON_RESET(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::EmergencyEventKind::Reset)

// ============================================================
// 🎚️ SLIDERS (Horizontal & Vertical)
// ============================================================

// Version simple
#define _ON_VALUE_CHANGED_1(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::ValueChanged)

// Version avec capture
#define _ON_VALUE_CHANGED_2(id, var_value) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::ValueChanged) \
        if (float var_value = e.value; true)

#define _ON_VALUE_CHANGED_DISPATCH(N) _INSTANTIOT_CONCAT(_ON_VALUE_CHANGED_, N)
#define ON_VALUE_CHANGED(...) _ON_VALUE_CHANGED_DISPATCH(_INSTANTIOT_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

// ValueChanging (temps réel pendant drag)
#define _ON_VALUE_CHANGING_1(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::ValueChanging)

#define _ON_VALUE_CHANGING_2(id, var_value) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::ValueChanging) \
        if (float var_value = e.value; true)

#define _ON_VALUE_CHANGING_DISPATCH(N) _INSTANTIOT_CONCAT(_ON_VALUE_CHANGING_, N)
#define ON_VALUE_CHANGING(...) _ON_VALUE_CHANGING_DISPATCH(_INSTANTIOT_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

// Drag events
#define ON_DRAG_STARTED(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::DragStarted)

#define ON_DRAG_ENDED(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SliderEventKind::DragEnded)

// ============================================================
// 🔘 SWITCH
// ============================================================

// Détecte ON (turnon OU setvalue avec isOn=true)
#define ON_TURN_ON(id) \
    if (_INSTANTIOT_IS_ID(e, id) && (e.kind == InstantIoT::SwitchEventKind::TurnOn || (e.kind == InstantIoT::SwitchEventKind::SetValue && e.isOn)))

// Détecte OFF (turnoff OU setvalue avec isOn=false)
#define ON_TURN_OFF(id) \
    if (_INSTANTIOT_IS_ID(e, id) && (e.kind == InstantIoT::SwitchEventKind::TurnOff || (e.kind == InstantIoT::SwitchEventKind::SetValue && !e.isOn)))

// Détecte toggle explicite
#define ON_SWITCH_TOGGLE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SwitchEventKind::Toggle)

// Avec capture de l'état (pour setvalue)
#define ON_SWITCH_VALUE(id, var_isOn) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SwitchEventKind::SetValue) \
        if (bool var_isOn = e.isOn; true)

// Alternative simple : juste vérifie isOn (marche avec tous les events)
#define ON_SWITCH_ON(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.isOn)

#define ON_SWITCH_OFF(id) \
    if (_INSTANTIOT_IS_ID(e, id) && !e.isOn)

// ============================================================
// 🕹️ JOYSTICK
// ============================================================

// Version simple
#define _ON_JOYSTICK_1(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::JoystickEventKind::PositionChanged)

// Version avec capture x, y (technique C++17 if-init)
#define _ON_JOYSTICK_3(id, var_x, var_y) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::JoystickEventKind::PositionChanged) \
        if (float var_x = e.x, var_y = e.y; true)

#define _ON_JOYSTICK_DISPATCH(N) _INSTANTIOT_CONCAT(_ON_JOYSTICK_, N)
#define ON_JOYSTICK(...) _ON_JOYSTICK_DISPATCH(_INSTANTIOT_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

// Alias
#define _ON_POSITION_CHANGED_1(id) _ON_JOYSTICK_1(id)
#define _ON_POSITION_CHANGED_3(id, var_x, var_y) _ON_JOYSTICK_3(id, var_x, var_y)
#define _ON_POSITION_CHANGED_DISPATCH(N) _INSTANTIOT_CONCAT(_ON_POSITION_CHANGED_, N)
#define ON_POSITION_CHANGED(...) _ON_POSITION_CHANGED_DISPATCH(_INSTANTIOT_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define ON_JOYSTICK_RELEASED(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::JoystickEventKind::Released)

// ============================================================
// 🎮 DIRECTION PAD
// ============================================================

// Press
#define ON_DPAD_UP(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Up && e.kind == InstantIoT::DPadEventKind::Press)

#define ON_DPAD_DOWN(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Down && e.kind == InstantIoT::DPadEventKind::Press)

#define ON_DPAD_LEFT(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Left && e.kind == InstantIoT::DPadEventKind::Press)

#define ON_DPAD_RIGHT(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Right && e.kind == InstantIoT::DPadEventKind::Press)

#define ON_DPAD_CENTER(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Center && e.kind == InstantIoT::DPadEventKind::Press)

// Release
#define ON_DPAD_UP_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Up && e.kind == InstantIoT::DPadEventKind::Release)

#define ON_DPAD_DOWN_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Down && e.kind == InstantIoT::DPadEventKind::Release)

#define ON_DPAD_LEFT_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Left && e.kind == InstantIoT::DPadEventKind::Release)

#define ON_DPAD_RIGHT_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Right && e.kind == InstantIoT::DPadEventKind::Release)

#define ON_DPAD_CENTER_RELEASE(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Center && e.kind == InstantIoT::DPadEventKind::Release)

// Long Press
#define ON_DPAD_UP_LONG(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Up && e.kind == InstantIoT::DPadEventKind::LongPress)

#define ON_DPAD_DOWN_LONG(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Down && e.kind == InstantIoT::DPadEventKind::LongPress)

#define ON_DPAD_LEFT_LONG(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Left && e.kind == InstantIoT::DPadEventKind::LongPress)

#define ON_DPAD_RIGHT_LONG(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Right && e.kind == InstantIoT::DPadEventKind::LongPress)

#define ON_DPAD_CENTER_LONG(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::Center && e.kind == InstantIoT::DPadEventKind::LongPress)

// Générique
#define ON_DPAD(id, btn) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::btn && e.kind == InstantIoT::DPadEventKind::Press)

#define ON_DPAD_RELEASE(id, btn) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::btn && e.kind == InstantIoT::DPadEventKind::Release)

#define ON_DPAD_LONG(id, btn) \
    if (_INSTANTIOT_IS_ID(e, id) && e.button == InstantIoT::DPadButton::btn && e.kind == InstantIoT::DPadEventKind::LongPress)

// ============================================================
// 🔀 SEGMENTED SWITCH
// ============================================================

// Version simple
#define _ON_SELECTION_CHANGED_1(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SegmentedEventKind::SelectionChanged)

// Version avec capture de l'index
#define _ON_SELECTION_CHANGED_2(id, var_index) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SegmentedEventKind::SelectionChanged) \
        if (int var_index = e.selectedIndex; true)

#define _ON_SELECTION_CHANGED_DISPATCH(N) _INSTANTIOT_CONCAT(_ON_SELECTION_CHANGED_, N)
#define ON_SELECTION_CHANGED(...) _ON_SELECTION_CHANGED_DISPATCH(_INSTANTIOT_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define ON_SEGMENT_SELECTED(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SegmentedEventKind::SegmentSelected)

#define ON_SEGMENT_DESELECTED(id) \
    if (_INSTANTIOT_IS_ID(e, id) && e.kind == InstantIoT::SegmentedEventKind::SegmentDeselected)

// ============================================================
// 📊 GENERIC WIDGET (ID check only)
// ============================================================

#define ON_WIDGET(id) \
    if (_INSTANTIOT_IS_ID(e, id))
