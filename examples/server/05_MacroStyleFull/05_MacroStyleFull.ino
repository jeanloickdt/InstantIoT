/*************************************************************
 * InstantIoT — Example: Macro-style FULL widget coverage
 *
 * Démontre ALL les blocs I<Widget> + WHEN_* de la lib :
 *   - SimpleButton     (btn1)   : WHEN_TOGGLED / PRESSED / RELEASED / LONG_PRESSED
 *   - AdvancedButton   (relay1) : WHEN_TOGGLED
 *   - HorizontalSlider (dim1)   : WHEN_CHANGING / CHANGED
 *   - VerticalSlider   (vol1)   : WHEN_CHANGING / CHANGED
 *   - Switch           (sw1)    : WHEN_TURNED_ON / TURNED_OFF / TOGGLED / SWITCH_SET
 *   - Joystick         (joy1)   : WHEN_MOVED / RELEASED
 *   - DirectionPad     (pad1)   : WHEN_PAD_PRESSED / PAD_RELEASED
 *   - SegmentedSwitch  (mode1)  : WHEN_SELECTION_CHANGED / SEGMENT_SELECTED
 *   - AdvancedChart    (chart1) : WHEN_REQUEST_DATA / REQUEST_REFRESH
 *
 * Board : ESP32
 *************************************************************/

#include <InstantIoTWiFiServer.hpp>

const char* WIFI_SSID      = "YOUR_WIFI_SSID";
const char* WIFI_PASS      = "YOUR_WIFI_PASSWORD";
const char* SERVER_IP      = "192.168.1.42";
const uint16_t SERVER_PORT = 9001;
const char* DEVICE_TOKEN   = "PASTE_YOUR_DEVICE_TOKEN";

#define LED_PIN   2
#define RELAY_PIN 4

InstantIoTWiFiServer instant(SERVER_IP, SERVER_PORT, DEVICE_TOKEN);

// ══════════════════════════════════════════════════════════════
//  BUTTONS
// ══════════════════════════════════════════════════════════════

ISimpleButton("btn1") {
    WHEN_TOGGLED(isOn) {
        analogWrite(LED_PIN, isOn ? 255 : 0);
        Serial.print("btn1 → "); Serial.println(isOn ? "ON" : "OFF");
    }
    WHEN_PRESSED      { Serial.println("btn1 pressed"); }
    WHEN_RELEASED     { Serial.println("btn1 released"); }
    WHEN_LONG_PRESSED { Serial.println("btn1 held"); }
};

IAdvancedButton("relay1") {
    WHEN_TOGGLED(on) {
        digitalWrite(RELAY_PIN, on ? HIGH : LOW);
        Serial.print("relay1 → "); Serial.println(on ? "ON" : "OFF");
    }
};

// ══════════════════════════════════════════════════════════════
//  SLIDERS
// ══════════════════════════════════════════════════════════════

IHorizontalSlider("dim1") {
    WHEN_CHANGING(v) {
        analogWrite(LED_PIN, (int)(v / 100.0f * 255.0f));
    }
    WHEN_CHANGED(v) {
        Serial.print("dim1 final → "); Serial.print(v); Serial.println("%");
    }
};

IVerticalSlider("vol1") {
    WHEN_CHANGING(v) {
        Serial.print("vol1 → "); Serial.println(v);
    }
    WHEN_CHANGED(v) {
        Serial.print("vol1 final → "); Serial.println(v);
    }
};

// ══════════════════════════════════════════════════════════════
//  SWITCH
// ══════════════════════════════════════════════════════════════

ISwitch("sw1") {
    WHEN_TURNED_ON        { Serial.println("sw1 → ON  (explicit turn-on)"); }
    WHEN_TURNED_OFF       { Serial.println("sw1 → OFF (explicit turn-off)"); }
    WHEN_TOGGLED(isOn)    { Serial.print("sw1 toggle → ");     Serial.println(isOn ? "ON" : "OFF"); }
    WHEN_SWITCH_SET(isOn) { Serial.print("sw1 set value → "); Serial.println(isOn ? "ON" : "OFF"); }
};

// ══════════════════════════════════════════════════════════════
//  JOYSTICK
// ══════════════════════════════════════════════════════════════

IJoystick("joy1") {
    WHEN_MOVED(x, y) {
        Serial.print("joy1 → x="); Serial.print(x, 1);
        Serial.print(" y=");       Serial.println(y, 1);
    }
    WHEN_RELEASED { Serial.println("joy1 released"); }
};

// ══════════════════════════════════════════════════════════════
//  DIRECTION PAD
// ══════════════════════════════════════════════════════════════

IDirectionPad("pad1") {
    WHEN_PAD_PRESSED(btn) {
        Serial.print("pad1 pressed: ");
        switch (btn) {
            case DPadButton::Up:     Serial.println("UP");     break;
            case DPadButton::Down:   Serial.println("DOWN");   break;
            case DPadButton::Left:   Serial.println("LEFT");   break;
            case DPadButton::Right:  Serial.println("RIGHT");  break;
            case DPadButton::Center: Serial.println("CENTER"); break;
            default:                 Serial.println("?");      break;
        }
    }
    WHEN_PAD_RELEASED(btn) {
        Serial.println("pad1 released");
    }
};

// ══════════════════════════════════════════════════════════════
//  SEGMENTED SWITCH
// ══════════════════════════════════════════════════════════════

ISegmentedSwitch("mode1") {
    WHEN_SELECTION_CHANGED(idx) {
        Serial.print("mode1 selection → index="); Serial.println(idx);
    }
    WHEN_SEGMENT_SELECTED(idx) {
        Serial.print("mode1 segment selected: "); Serial.println(idx);
    }
    WHEN_SEGMENT_DESELECTED(idx) {
        Serial.print("mode1 segment deselected: "); Serial.println(idx);
    }
};

// ══════════════════════════════════════════════════════════════
//  ADVANCED CHART (requestData / requestRefresh)
// ══════════════════════════════════════════════════════════════

IAdvancedChart("chart1") {
    WHEN_REQUEST_DATA {
        Serial.println("chart1 requestData → push historical series here");
        // ex: instant.chart("chart1").addPoint("s1", ...);
    }
    WHEN_REQUEST_REFRESH {
        Serial.println("chart1 requestRefresh → re-send latest points");
    }
};

// ══════════════════════════════════════════════════════════════
//  SETUP / LOOP
// ══════════════════════════════════════════════════════════════

void setup() {
    delay(2000);
    Serial.begin(115200);
    instant.setHeartbeat(1000);

    pinMode(LED_PIN,   OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    analogWrite(LED_PIN, 0);
    digitalWrite(RELAY_PIN, LOW);

    Serial.println();
    Serial.println("=== Macro-style FULL — all widgets ===");
    Serial.print("Server: "); Serial.print(SERVER_IP);
    Serial.print(":");        Serial.println(SERVER_PORT);

    if (!instant.begin(WIFI_SSID, WIFI_PASS)) {
        Serial.println("[ERROR] Initial connect failed, will keep retrying.");
    } else {
        Serial.print("Local IP: "); Serial.println(instant.getLocalIP());
        Serial.println("[OK] Connected");
    }

    Serial.println("Widgets: btn1, relay1, dim1, vol1, sw1, joy1, pad1, mode1, chart1");
}

void loop() {
    instant.loop();
}
