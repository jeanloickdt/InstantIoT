#pragma once
/**
 * ============================================================
 * 📋 Registry.hpp - Dispatch des événements widgets
 * ============================================================
 */

#include <Arduino.h>
#include "InstantIoTMessage.hpp"
#include "Codec.h"

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
        const char* widgetType,
        const char* widgetId,
        const char* event,
        const DecodedMessage& msg
    ) {
        if (!widgetType || !widgetId || !event) return;
        
        // SIMPLE BUTTON
        if (strcmp(widgetType, "simplebutton") == 0) {
            SimpleButtonEvent e;
            e.widgetId = widgetId;
            e.isOn = msg.getParamBool("isOn", false);
            
            if (strcmp(event, "press") == 0) e.kind = ButtonEventKind::Press;
            else if (strcmp(event, "release") == 0) e.kind = ButtonEventKind::Release;
            else if (strcmp(event, "longpress") == 0) e.kind = ButtonEventKind::LongPress;
            else if (strcmp(event, "toggle") == 0) e.kind = ButtonEventKind::Toggle;
            else return;
            
            onSimpleButtonEvent(e);
            return;
        }
        
        // ADVANCED BUTTON
        if (strcmp(widgetType, "advancedbutton") == 0) {
            AdvancedButtonEvent e;
            e.widgetId = widgetId;
            e.isOn = msg.getParamBool("isOn", false);
            
            if (strcmp(event, "press") == 0) e.kind = ButtonEventKind::Press;
            else if (strcmp(event, "release") == 0) e.kind = ButtonEventKind::Release;
            else if (strcmp(event, "longpress") == 0) e.kind = ButtonEventKind::LongPress;
            else if (strcmp(event, "toggle") == 0) e.kind = ButtonEventKind::Toggle;
            else return;
            
            onAdvancedButtonEvent(e);
            return;
        }
        
        // HORIZONTAL SLIDER
        if (strcmp(widgetType, "horizontalslider") == 0) {
            HorizontalSliderEvent e;
            e.widgetId = widgetId;
            e.value = msg.getParamFloat("value", 0.0f);
            e.startValue = msg.getParamFloat("startValue", 0.0f);
            e.finalValue = msg.getParamFloat("finalValue", 0.0f);
            
            if (strcmp(event, "valuechanging") == 0) e.kind = SliderEventKind::ValueChanging;
            else if (strcmp(event, "valuechanged") == 0) e.kind = SliderEventKind::ValueChanged;
            else if (strcmp(event, "dragstarted") == 0) e.kind = SliderEventKind::DragStarted;
            else if (strcmp(event, "dragended") == 0) e.kind = SliderEventKind::DragEnded;
            else return;
            
            onHorizontalSliderEvent(e);
            return;
        }
        
        // VERTICAL SLIDER
        if (strcmp(widgetType, "verticalslider") == 0) {
            VerticalSliderEvent e;
            e.widgetId = widgetId;
            e.value = msg.getParamFloat("value", 0.0f);
            e.startValue = msg.getParamFloat("startValue", 0.0f);
            e.finalValue = msg.getParamFloat("finalValue", 0.0f);
            
            if (strcmp(event, "valuechanging") == 0) e.kind = SliderEventKind::ValueChanging;
            else if (strcmp(event, "valuechanged") == 0) e.kind = SliderEventKind::ValueChanged;
            else if (strcmp(event, "dragstarted") == 0) e.kind = SliderEventKind::DragStarted;
            else if (strcmp(event, "dragended") == 0) e.kind = SliderEventKind::DragEnded;
            else return;
            
            onVerticalSliderEvent(e);
            return;
        }
        
        // SWITCH
        if (strcmp(widgetType, "switch") == 0) {
            SwitchEvent e;
            e.widgetId = widgetId;
            e.isOn = msg.getParamBool("isOn", false);

            if (strcmp(event, "turnon") == 0) {
                e.kind = SwitchEventKind::TurnOn;
                e.isOn = true;
            }
            else if (strcmp(event, "turnoff") == 0) {
                e.kind = SwitchEventKind::TurnOff;
                e.isOn = false;
            }
            else if (strcmp(event, "toggle") == 0) {
                e.kind = SwitchEventKind::Toggle;
            }
            else if (strcmp(event, "setvalue") == 0) {
                e.kind = SwitchEventKind::SetValue;
                // isOn déjà parsé depuis payload
            }
            else return;

            onSwitchEvent(e);
            return;
        }
        
        // JOYSTICK
        if (strcmp(widgetType, "joystick") == 0) {
            JoystickEvent e;
            e.widgetId = widgetId;
            e.x = msg.getParamFloat("x", 0.0f);
            e.y = msg.getParamFloat("y", 0.0f);
            
            if (strcmp(event, "positionchanged") == 0) e.kind = JoystickEventKind::PositionChanged;
            else if (strcmp(event, "released") == 0) { e.kind = JoystickEventKind::Released; e.x = 0; e.y = 0; }
            else return;
            
            onJoystickEvent(e);
            return;
        }
        
        // DIRECTION PAD
        if (strcmp(widgetType, "directionpad") == 0) {
            DirectionPadEvent e;
            e.widgetId = widgetId;
            e.button = DPadButton::Unknown;
            e.buttonName = "";
            
            const char* buttonStr = msg.getParam("button");
            if (buttonStr) {
                e.buttonName = buttonStr;
                if (strcmp(buttonStr, "up") == 0) e.button = DPadButton::Up;
                else if (strcmp(buttonStr, "down") == 0) e.button = DPadButton::Down;
                else if (strcmp(buttonStr, "left") == 0) e.button = DPadButton::Left;
                else if (strcmp(buttonStr, "right") == 0) e.button = DPadButton::Right;
                else if (strcmp(buttonStr, "center") == 0) e.button = DPadButton::Center;
            }
            
            if (strcmp(event, "buttonpressed") == 0) e.kind = DPadEventKind::Press;
            else if (strcmp(event, "buttonreleased") == 0) e.kind = DPadEventKind::Release;
            else if (strcmp(event, "buttonlongpressed") == 0) e.kind = DPadEventKind::LongPress;
            else return;
            
            onDirectionPadEvent(e);
            return;
        }
        
        // SEGMENTED SWITCH
        if (strcmp(widgetType, "segmentedswitch") == 0) {
            SegmentedSwitchEvent e;
            e.widgetId = widgetId;
            e.selectedIndex = msg.getParamInt("index", -1);  // ✅ AJOUTÉ: lire "index" du payload
            e.selectedIds = msg.getParam("indices");          // ✅ CHANGÉ: "indices" au lieu de "selectedIds"
            e.segmentId = msg.getParam("segmentId");
            e.count = msg.getParamInt("count", 0);

            if (strcmp(event, "selectionchanged") == 0) e.kind = SegmentedEventKind::SelectionChanged;
            else if (strcmp(event, "segmentselected") == 0) e.kind = SegmentedEventKind::SegmentSelected;
            else if (strcmp(event, "segmentdeselected") == 0) e.kind = SegmentedEventKind::SegmentDeselected;
            else return;

            onSegmentedSwitchEvent(e);
            return;
        }
        
        // EMERGENCY BUTTON
        if (strcmp(widgetType, "emergencybutton") == 0) {
            EmergencyButtonEvent e;
            e.widgetId = widgetId;
            
            if (strcmp(event, "trigger") == 0) e.kind = EmergencyEventKind::Trigger;
            else if (strcmp(event, "reset") == 0) e.kind = EmergencyEventKind::Reset;
            else return;
            
            onEmergencyButtonEvent(e);
            return;
        }
        
        // GENERIC REQUEST
        if (strcmp(event, "request") == 0 || strcmp(event, "refresh") == 0) {
            WidgetRequest e;
            e.widgetId = widgetId;
            e.requestType = event;
            onWidgetRequest(e);
        }
    }
};

} // namespace InstantIoT
