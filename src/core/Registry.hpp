#pragma once
/**
 * ============================================================
 * 📋 Registry.hpp - Dispatch des événements widgets
 *
 * Travaille directement avec les codes uint8 du protocole
 * binaire iWidgets v1 — zéro strcmp, zéro strings.
 * ============================================================
 */

#include <Arduino.h>
#include "InstantIoTMessage.hpp"
#include "BinaryCodec.hpp"

// ============================================================
// 🔗 WEAK CALLBACK DECLARATIONS
// ============================================================

__attribute__((weak)) void onSimpleButtonEvent(const SimpleButtonEvent& e);
__attribute__((weak)) void onAdvancedButtonEvent(const AdvancedButtonEvent& e);
__attribute__((weak)) void onEmergencyButtonEvent(const EmergencyButtonEvent& e);
__attribute__((weak)) void onHorizontalSliderEvent(const HorizontalSliderEvent& e);
__attribute__((weak)) void onVerticalSliderEvent(const VerticalSliderEvent& e);
__attribute__((weak)) void onSwitchEvent(const SwitchEvent& e);
__attribute__((weak)) void onJoystickEvent(const JoystickEvent& e);
__attribute__((weak)) void onDirectionPadEvent(const DirectionPadEvent& e);
__attribute__((weak)) void onSegmentedSwitchEvent(const SegmentedSwitchEvent& e);
__attribute__((weak)) void onWidgetRequest(const WidgetRequest& e);

namespace InstantIoT {

class WidgetRegistry {
public:
    static void dispatch(
        uint8_t typeCode,
        const char* widgetId,
        uint8_t eventCode,
        const DecodedMessage& msg
    ) {
        if (!widgetId) return;

        switch (typeCode) {

            // ── SimpleButton ──────────────────────────────────
            case TYPE_SIMPLEBUTTON: {
                SimpleButtonEvent e;
                e.widgetId = widgetId;
                e.isOn     = msg.getParamBool("state", false);
                switch (eventCode) {
                    case CMD_PRESS:          e.kind = ButtonEventKind::Press;     break;
                    case CMD_RELEASE:        e.kind = ButtonEventKind::Release;   break;
                    case CMD_LONGPRESS:      e.kind = ButtonEventKind::LongPress; break;
                    case CMD_TOGGLE:         e.kind = ButtonEventKind::Toggle;    break;
                    default: return;
                }
                onSimpleButtonEvent(e);
                return;
            }

            // ── AdvancedButton ────────────────────────────────
            case TYPE_ADVANCEDBUTTON: {
                AdvancedButtonEvent e;
                e.widgetId = widgetId;
                e.isOn     = msg.getParamBool("state", false);
                switch (eventCode) {
                    case CMD_PRESS:     e.kind = ButtonEventKind::Press;     break;
                    case CMD_RELEASE:   e.kind = ButtonEventKind::Release;   break;
                    case CMD_LONGPRESS: e.kind = ButtonEventKind::LongPress; break;
                    case CMD_TOGGLE:    e.kind = ButtonEventKind::Toggle;    break;
                    default: return;
                }
                onAdvancedButtonEvent(e);
                return;
            }

            // ── HorizontalSlider ──────────────────────────────
            case TYPE_HSLIDER: {
                HorizontalSliderEvent e;
                e.widgetId   = widgetId;
                e.value      = msg.getParamFloat("value", 0.0f);
                e.startValue = msg.getParamFloat("startValue", 0.0f);
                e.finalValue = msg.getParamFloat("finalValue", 0.0f);
                switch (eventCode) {
                    case CMD_VALUECHANGING: e.kind = SliderEventKind::ValueChanging; break;
                    case CMD_VALUECHANGED:  e.kind = SliderEventKind::ValueChanged;  break;
                    case CMD_DRAGSTARTED:   e.kind = SliderEventKind::DragStarted;   break;
                    case CMD_DRAGENDED:     e.kind = SliderEventKind::DragEnded;     break;
                    default: return;
                }
                onHorizontalSliderEvent(e);
                return;
            }

            // ── VerticalSlider ────────────────────────────────
            case TYPE_VSLIDER: {
                VerticalSliderEvent e;
                e.widgetId   = widgetId;
                e.value      = msg.getParamFloat("value", 0.0f);
                e.startValue = msg.getParamFloat("startValue", 0.0f);
                e.finalValue = msg.getParamFloat("finalValue", 0.0f);
                switch (eventCode) {
                    case CMD_VALUECHANGING: e.kind = SliderEventKind::ValueChanging; break;
                    case CMD_VALUECHANGED:  e.kind = SliderEventKind::ValueChanged;  break;
                    case CMD_DRAGSTARTED:   e.kind = SliderEventKind::DragStarted;   break;
                    case CMD_DRAGENDED:     e.kind = SliderEventKind::DragEnded;     break;
                    default: return;
                }
                onVerticalSliderEvent(e);
                return;
            }

            // ── Switch ────────────────────────────────────────
            case TYPE_SWITCH: {
                SwitchEvent e;
                e.widgetId = widgetId;
                switch (eventCode) {
                    case CMD_TURNON:
                        e.kind = SwitchEventKind::TurnOn;
                        e.isOn = true;
                        break;
                    case CMD_TURNOFF:
                        e.kind = SwitchEventKind::TurnOff;
                        e.isOn = false;
                        break;
                    case 0x03:
                        e.kind = SwitchEventKind::Toggle;
                        e.isOn = msg.getParamBool("value", false);
                        break;
                    case CMD_SWITCHVALUE:
                        e.kind = SwitchEventKind::SetValue;
                        e.isOn = msg.getParamBool("value", false);
                        break;
                    default: return;
                }
                onSwitchEvent(e);
                return;
            }

            // ── Joystick ──────────────────────────────────────
            case TYPE_JOYSTICK: {
                JoystickEvent e;
                e.widgetId = widgetId;
                e.x        = msg.getParamFloat("x", 0.0f);
                e.y        = msg.getParamFloat("y", 0.0f);
                switch (eventCode) {
                    case CMD_POSCHANGED:
                        e.kind = JoystickEventKind::PositionChanged;
                        break;
                    case CMD_RELEASED:
                        e.kind = JoystickEventKind::Released;
                        e.x = 0; e.y = 0;
                        break;
                    default: return;
                }
                onJoystickEvent(e);
                return;
            }

           // ── DirectionPad ──────────────────────────────────
            case TYPE_DIRECTIONPAD: {
                DirectionPadEvent e;
                e.widgetId  = widgetId;
                e.buttonName = "";

                uint8_t btnCode = (uint8_t)msg.getParamInt("button", 255);
                switch (btnCode) {
                    case 0x00: e.button = DPadButton::Up;     e.buttonName = "up";     break;
                    case 0x01: e.button = DPadButton::Down;   e.buttonName = "down";   break;
                    case 0x02: e.button = DPadButton::Left;   e.buttonName = "left";   break;
                    case 0x03: e.button = DPadButton::Right;  e.buttonName = "right";  break;
                    case 0x04: e.button = DPadButton::Center; e.buttonName = "center"; break;
                    default:   e.button = DPadButton::Unknown;                          break;
                }

                switch (eventCode) {
                    case CMD_BTNPRESSED:     e.kind = DPadEventKind::Press;     break;
                    case CMD_BTNRELEASED:    e.kind = DPadEventKind::Release;   break;
                    case CMD_BTNLONGPRESSED: e.kind = DPadEventKind::LongPress; break;
                    default: return;
                }
                onDirectionPadEvent(e);
                return;
            }

            // ── SegmentedSwitch ───────────────────────────────
            case TYPE_SEGSWITCH: {
                SegmentedSwitchEvent e;
                e.widgetId     = widgetId;
                e.selectedIndex = msg.getParamInt("index", -1);
                e.selectedIds  = msg.getParam("ids");
                e.segmentId    = msg.getParam("optionId");
                e.count        = msg.getParamInt("count", 0);

                switch (eventCode) {
                    case CMD_SELCHANGED:   e.kind = SegmentedEventKind::SelectionChanged;  break;
                    case CMD_SEGSELECTED:  e.kind = SegmentedEventKind::SegmentSelected;   break;
                    case CMD_SEGDESELECTED: e.kind = SegmentedEventKind::SegmentDeselected; break;
                    default: return;
                }
                onSegmentedSwitchEvent(e);
                return;
            }

            // ── AdvancedChart requestData/requestRefresh ──────
            case TYPE_ADVANCEDCHART: {
                if (eventCode == 0x10 || eventCode == 0x11) {
                    WidgetRequest e;
                    e.widgetId    = widgetId;
                    e.requestType = (eventCode == 0x10) ? "requestdata" : "requestrefresh";
                    onWidgetRequest(e);
                }
                return;
            }

            // ── EmergencyButton ───────────────────────────────
            // Note: EmergencyButton n'est pas dans le protocole v1
            // mais gardé pour compatibilité
            default:
                return;
        }
    }

    // ════════════════════════════════════════════════════════
    // Surcharge string → conservée pour compatibilité
    // mais délègue vers la version uint8
    // ════════════════════════════════════════════════════════
    static void dispatch(
        const char* widgetType,
        const char* widgetId,
        const char* event,
        const DecodedMessage& msg
    ) {
        // Cette surcharge ne devrait plus être appelée en mode binaire
        // mais on la garde pour ne pas casser d'éventuels usages legacy
        (void)widgetType; (void)widgetId; (void)event; (void)msg;
    }
};

} // namespace InstantIoT