/*************************************************************
 * TEST: SegmentedSwitch
 * 
 * Events reçus de l'app:
 * - selectionchanged (payload: selectedIds, count)
 * - segmentselected (payload: segmentId)
 * - segmentdeselected (payload: segmentId)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>

InstantIoTWiFiAP instant("Test_Segmented", "12345678");

void onSegmentedSwitchEvent(const SegmentedSwitchEvent& e) {
    Serial.print("[Segmented] id=");
    Serial.print(e.widgetId);
    
    ON_SELECTION_CHANGED("seg1") {
        Serial.print(" SELECTION_CHANGED ids=");
        Serial.print(e.selectedIds ? e.selectedIds : "null");
        Serial.print(" count=");
        Serial.println(e.count);
    }
    
    ON_SEGMENT_SELECTED("seg1") {
        Serial.print(" SEGMENT_SELECTED=");
        Serial.println(e.segmentId ? e.segmentId : "null");
    }
    
    ON_SEGMENT_DESELECTED("seg1") {
        Serial.print(" SEGMENT_DESELECTED=");
        Serial.println(e.segmentId ? e.segmentId : "null");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TEST: SegmentedSwitch ===");
    Serial.println("Widget requis: SegmentedSwitch id='seg1'");
    instant.begin();
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
}
