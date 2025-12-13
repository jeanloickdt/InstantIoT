/*************************************************************
 * TEST: AdvancedChart Widget
 * 
 * Events envoyés vers l'app:
 * - adddatapoint (payload: seriesId, x, y)
 * - clearseriesdata (payload: seriesId)
 * - clearalldata
 * - addseries (payload: seriesId, label, lineColor)
 * - updateseriesstyle (payload: seriesId, lineColor, showPoints, showFill)
 * - setmaxdatapoints (payload: maxDataPoints)
 * - updateaxislabels (payload: chartTitle, xAxisLabel, yAxisLabel)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_Chart", "12345678");
InstantTimer timers;

// Fake sensor data
float voltage = 220.0;
float current = 5.0;
float power = 0;

void sendVoltage() {
    if (!instant.connected()) return;
    voltage += random(-50, 51) / 10.0;
    voltage = constrain(voltage, 200, 240);
    instant.chart("chart1").addPoint("U", voltage);
}

void sendCurrent() {
    if (!instant.connected()) return;
    current += random(-20, 21) / 10.0;
    current = constrain(current, 0, 10);
    instant.chart("chart1").addPoint("I", current);
}

void sendPower() {
    if (!instant.connected()) return;
    power = voltage * current / 100.0;  // Scale for display
    instant.chart("chart1").addPoint("P", power);
    
    Serial.print("U=");
    Serial.print(voltage);
    Serial.print(" I=");
    Serial.print(current);
    Serial.print(" P=");
    Serial.println(power);
}

void setup() {
     delay(3000);
    Serial.begin(115200);
    delay(1000);
    randomSeed(analogRead(0));
    Serial.println("\n=== TEST: AdvancedChart Widget ===");
    Serial.println("Widget requis: AdvancedChart id='chart1'");
    Serial.println("Series auto-created: U, I, P");
    instant.begin();
    
    // Staggered sends to avoid congestion
    timers.every(200, sendVoltage);
    timers.every(210, sendCurrent);
    timers.every(220, sendPower);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
