#pragma once
/**
 * ============================================================
 * 📋 Registry.hpp - Widget event dispatch
 *
 * Works directly with the uint8 codes of the iWidgets v1
 * binary protocol — zero strcmp, zero strings.
 * ============================================================
 */

#include <Arduino.h>
#include <string.h>
#include "InstantIoTMessage.hpp"
#include "BinaryCodec.hpp"
#include "InstantIoTSignals.hpp"
#include "SignalEvents.hpp"

// ============================================================
// 📚 PER-WIDGET HANDLER REGISTRY (used by I<Widget>(id) macros)
//
// These templates allow registering a handler per widget id
// via style macros:
//
//   ISimpleButton("btn1") { WHEN_TOGGLED(on) { ... } };
//
// Each block becomes:
//   - 1 static function void _fn(const EventT&)
//   - 1 static global WidgetRegistrar<EventT> that attaches its node
//     to the linked list at startup (ctor before setup())
//
// RAM: 12 B per block (ESP32, 3 ptrs × 4 B). No heap.
// Dispatch cost: 1 strcmp per registered handler of the relevant type.
// ============================================================
namespace InstantIoT {

/**
 * Un bloc enregistré.
 *
 * Deux clés possibles, jamais les deux à la fois :
 *  - `widgetId` non nul  → adressé par NOM, l'ancien modèle, gelé.
 *  - `widgetId` nul      → adressé par ADRESSE, `address` fait foi.
 *
 * La cohabitation n'est pas une hésitation : elle laisse les croquis
 * existants compiler et tourner pendant que les nouveaux passent aux
 * adresses. Le jour où plus personne n'utilise les noms, il n'y a qu'un
 * champ à retirer.
 */
template<typename EventT>
struct WidgetHandler {
    const char* widgetId;
    uint8_t     address;
    void (*fn)(const EventT&);
    WidgetHandler<EventT>* next;
};

template<typename EventT>
inline WidgetHandler<EventT>*& handlerListHead() {
    static WidgetHandler<EventT>* head = nullptr;
    return head;
}

// ============================================================
// 🗺️ ADRESSE → TYPE DE WIDGET
//
// C'est ici que se règle « comment la lib sait que I5 est un
// SimpleButton » : elle ne le devine pas et la trame ne le dit
// pas — le bloc du croquis l'a inscrit au démarrage.
//
// Le sens de lecture appartient donc au code que l'utilisateur
// relit, pas à un réglage distant qu'il ne voit pas. C'est
// l'inverse du modèle où la trame annonce le type : là-bas,
// changer un widget dans l'app change le sens du croquis sans
// qu'une ligne ait bougé.
// ============================================================

struct AddressType {
    uint8_t      address;
    uint8_t      typeCode;
    AddressType* next;
};

inline AddressType*& addressTypeHead() {
    static AddressType* head = nullptr;
    return head;
}

/** Le type déclaré à cette adresse, ou 0 si aucun bloc ne l'écoute. */
inline uint8_t typeAtAddress(uint8_t address) {
    for (AddressType* n = addressTypeHead(); n; n = n->next) {
        if (n->address == address) return n->typeCode;
    }
    return 0;
}

template<typename EventT>
struct WidgetRegistrar {
    WidgetHandler<EventT> node;
    AddressType           typeNode;

    /** Adressé par NOM — l'ancien modèle. */
    WidgetRegistrar(const char* id, uint8_t /*typeCode*/, void (*fn)(const EventT&)) {
        node.widgetId = id;
        node.address  = 0;
        node.fn       = fn;
        node.next     = handlerListHead<EventT>();
        handlerListHead<EventT>() = &node;
        typeNode.next = nullptr;   // rien à inscrire : pas d'adresse
    }

    /**
     * Adressé par ADRESSE.
     *
     * Le constructeur fait DEUX choses : il accroche le gestionnaire, et il
     * inscrit « à cette adresse vit ce type de widget ». Écrire le bloc EST
     * l'enregistrement — rien n'est alloué, les deux nœuds sont membres de ce
     * registrar, lui-même une variable globale que le compilateur place.
     */
    WidgetRegistrar(SignalRef ref, uint8_t typeCode, void (*fn)(const EventT&)) {
        node.widgetId = nullptr;
        node.address  = ref.addr;
        node.fn       = fn;
        node.next     = handlerListHead<EventT>();
        handlerListHead<EventT>() = &node;

        typeNode.address  = ref.addr;
        typeNode.typeCode = typeCode;
        typeNode.next     = addressTypeHead();
        addressTypeHead() = &typeNode;
    }
};

/**
 * Remet l'événement aux blocs concernés.
 *
 * [address] non nul : on livre aux blocs adressés par octet. Nul : aux blocs
 * adressés par nom. Un même événement ne traverse jamais les deux familles —
 * sans quoi un bloc nommé « 5 » recevrait ce qui va à l'adresse 5.
 */
template<typename EventT>
inline void deliver(const EventT& e, const uint8_t* address) {
    for (auto* h = handlerListHead<EventT>(); h; h = h->next) {
        const bool matches = address
            ? (h->widgetId == nullptr && h->address == *address)
            : (h->widgetId != nullptr && e.widgetId && strcmp(h->widgetId, e.widgetId) == 0);
        if (matches) h->fn(e);
    }
}

/** Livraison par nom — conservée pour les appels existants. */
template<typename EventT>
inline void dispatchToHandlers(const EventT& e) {
    deliver(e, nullptr);
}

} // namespace InstantIoT

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

namespace InstantIoT {

class WidgetRegistry {
public:
    /**
     * @param address non nul quand la trame était adressée par octet. Le gros
     *        `switch` ci-dessous ne change pas d'un cas : seule la LIVRAISON
     *        finale sait s'il faut comparer des noms ou des adresses.
     */
    static void dispatch(
        uint8_t typeCode,
        const char* widgetId,
        uint8_t eventCode,
        const DecodedMessage& msg,
        const uint8_t* address = nullptr
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
                InstantIoT::deliver(e, address);
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
                InstantIoT::deliver(e, address);
                return;
            }

            // ── HorizontalSlider ──────────────────────────────
            case TYPE_HSLIDER: {
                HorizontalSliderEvent e;
                e.widgetId = widgetId;
                e.value    = msg.getParamFloat("value", 0.0f);
                switch (eventCode) {
                    case CMD_VALUECHANGING: e.kind = SliderEventKind::ValueChanging; break;
                    case CMD_VALUECHANGED:  e.kind = SliderEventKind::ValueChanged;  break;
                    case CMD_DRAGSTARTED:   e.kind = SliderEventKind::DragStarted;   break;
                    case CMD_DRAGENDED:     e.kind = SliderEventKind::DragEnded;     break;
                    default: return;
                }
                onHorizontalSliderEvent(e);
                InstantIoT::deliver(e, address);
                return;
            }

            // ── VerticalSlider ────────────────────────────────
            case TYPE_VSLIDER: {
                VerticalSliderEvent e;
                e.widgetId = widgetId;
                e.value    = msg.getParamFloat("value", 0.0f);
                switch (eventCode) {
                    case CMD_VALUECHANGING: e.kind = SliderEventKind::ValueChanging; break;
                    case CMD_VALUECHANGED:  e.kind = SliderEventKind::ValueChanged;  break;
                    case CMD_DRAGSTARTED:   e.kind = SliderEventKind::DragStarted;   break;
                    case CMD_DRAGENDED:     e.kind = SliderEventKind::DragEnded;     break;
                    default: return;
                }
                onVerticalSliderEvent(e);
                InstantIoT::deliver(e, address);
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
                InstantIoT::deliver(e, address);
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
                InstantIoT::deliver(e, address);
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
                InstantIoT::deliver(e, address);
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
                InstantIoT::deliver(e, address);
                return;
            }

            // ── EmergencyButton ───────────────────────────────
            case TYPE_EMERGENCYBUTTON: {
                EmergencyButtonEvent e;
                e.widgetId = widgetId;
                switch (eventCode) {
                    case CMD_EMERGENCY_TRIGGER: e.kind = EmergencyEventKind::Trigger; break;
                    case CMD_EMERGENCY_RESET:   e.kind = EmergencyEventKind::Reset;   break;
                    default: return;
                }
                onEmergencyButtonEvent(e);
                InstantIoT::deliver(e, address);
                return;
            }

            default:
                return;
        }
    }

    // ════════════════════════════════════════════════════════
    // String overload → kept for compatibility
    // but delegates to the uint8 version
    // ════════════════════════════════════════════════════════
    static void dispatch(
        const char* widgetType,
        const char* widgetId,
        const char* event,
        const DecodedMessage& msg
    ) {
        // This overload should no longer be called in binary mode
        // but we keep it so as not to break any legacy usages
        (void)widgetType; (void)widgetId; (void)event; (void)msg;
    }
};

} // namespace InstantIoT