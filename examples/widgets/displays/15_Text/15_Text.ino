/*************************************************************
 * TEST: Text Widget
 * 
 * Events envoyés vers l'app:
 * - settext (payload: text)
 * - setcolor (payload: color hex)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_Text", "12345678");
InstantTimer timers;

int msgIndex = 0;
const char* messages[] = {
    "Hello World!",
    "InstantIoT",
    "Temperature: 25C",
    "Status: OK",
    "Connecting...",
    "Ready!"
};

uint32_t colors[] = {
    0xFFFFFF,  // White
    0xFF0000,  // Red
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0xFFFF00,  // Yellow
    0xFF00FF   // Magenta
};

void testText() {
    if (!instant.connected()) return;
    
    Serial.print("Text: setText('");
    Serial.print(messages[msgIndex]);
    Serial.println("')");
    
    instant.text("text1").setText(messages[msgIndex]);
    delay(1);
    instant.text("text1") .setColor(colors[msgIndex]);
    
    msgIndex = (msgIndex + 1) % 6;
}

void setup() {
      delay(3000);
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST: Text Widget ===");
    Serial.println("Widget requis: Text id='text1'");
    instant.begin();
    
    timers.every(1500, testText);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}
