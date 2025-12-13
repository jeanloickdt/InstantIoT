/*************************************************************
 * TEST: LED Widget - TOUTES LES MÉTHODES
 * 
 * Events envoyés vers l'app:
 * - turnon
 * - turnoff
 * - toggle
 * - setbrightness (payload: brightness 0.0-1.0)
 * - updateoncolors (payload: led, halo, rays - hex colors)
 * - showhalo (payload: show true/false)
 * - showrays (payload: show true/false)
 *************************************************************/

#include <InstantIoTWiFiAP.hpp>
#include <utils/InstantIoTTimer.hpp>

InstantIoTWiFiAP instant("Test_LED", "12345678");
InstantTimer timers;

int testPhase = 0;

void testLED() {
    if (!instant.connected()) return;
    
    switch (testPhase) {
        // === ON/OFF/TOGGLE ===
        case 0:
            Serial.println("LED: turnOn()");
            instant.led("led1").on();
            break;
        case 1:
            Serial.println("LED: turnOff()");
            instant.led("led1").off();
            break;
        case 2:
            Serial.println("LED: toggle()");
            instant.led("led1").toggle();
            break;
            
        // === BRIGHTNESS ===
        case 3:
            Serial.println("LED: setIntensity(0.2)");
            instant.led("led1").on();
            instant.led("led1").setIntensity(0.2);
            break;
        case 4:
            Serial.println("LED: setIntensity(0.5)");
            instant.led("led1").setIntensity(0.5);
            break;
        case 5:
            Serial.println("LED: setBrightness(1.0)");
            instant.led("led1").setBrightness(1.0);
            break;
            
        // === COULEURS ===
        case 6:
            Serial.println("LED: setColor(RED)");
            instant.led("led1").setColor(0xFF0000);
            break;
        case 7:
            Serial.println("LED: setColor(GREEN)");
            instant.led("led1").setColor(0x00FF00);
            break;
        case 8:
            Serial.println("LED: setColor(BLUE)");
            instant.led("led1").setColor(0x0000FF);
            break;
        case 9:
            Serial.println("LED: setColor(YELLOW)");
            instant.led("led1").setColor(0xFFFF00);
            break;
        case 10:
            Serial.println("LED: setColor(MAGENTA)");
            instant.led("led1").setColor(0xFF00FF);
            break;
        case 11:
            Serial.println("LED: setColor(CYAN)");
            instant.led("led1").setColor(0x00FFFF);
            break;
            
        // === COULEURS COMPLÈTES (led + halo + rays) ===
        case 12:
            Serial.println("LED: setColors(RED led, ORANGE halo, YELLOW rays)");
            instant.led("led1").setColors(0xFF0000, 0xFF8800, 0xFFFF00);
            break;
        case 13:
            Serial.println("LED: setColors(BLUE led, CYAN halo, WHITE rays)");
            instant.led("led1").setColors(0x0000FF, 0x00FFFF, 0xFFFFFF);
            break;
            
        // === HALO & RAYS ===
        case 14:
            Serial.println("LED: showHalo(true)");
            instant.led("led1").showHalo(true);
            break;
        case 15:
            Serial.println("LED: showHalo(false)");
            instant.led("led1").showHalo(false);
            break;
        case 16:
            Serial.println("LED: showRays(true)");
            instant.led("led1").showRays(true);
            break;
        case 17:
            Serial.println("LED: showRays(false)");
            instant.led("led1").showRays(false);
            break;
            
        // === COMBINÉ ===
        case 18:
            Serial.println("LED: setState(true, 0.3)");
            instant.led("led1").setState(true, 0.3);
            break;
        case 19:
            Serial.println("LED: setState(true, 1.0)");
            instant.led("led1").setState(true, 1.0);
            break;
        case 20:
            Serial.println("LED: setState(false)");
            instant.led("led1").setState(false);
            break;
            
        // === CHAÎNAGE ===
        case 21:
            Serial.println("LED: Chaînage - turnOn + setColor + setBrightness");
            instant.led("led1")
                .turnOn()
                .setColor(0x00FF00)
                .setBrightness(0.8);
            break;
        case 22:
            Serial.println("LED: Chaînage - setColors + showHalo + showRays");
            instant.led("led1")
                .setColors(0xFF0000, 0xFF4400, 0xFFAA00)
                .showHalo(true)
                .showRays(true);
            break;
    }
    
    testPhase = (testPhase + 1) % 23;
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== TEST: LED Widget - COMPLET ===");
    Serial.println("Widget requis: LED id='led1'");
    Serial.println("");
    Serial.println("Méthodes testées:");
    Serial.println("  - turnOn(), turnOff(), toggle()");
    Serial.println("  - setIntensity(float), setBrightness(float)");
    Serial.println("  - setColor(uint32_t rgb)");
    Serial.println("  - setColors(led, halo, rays)");
    Serial.println("  - showHalo(bool), showRays(bool)");
    Serial.println("  - setState(bool on, float intensity)");
    Serial.println("  - Chaînage de méthodes");
    Serial.println("");
    
    instant.begin();
    
    timers.every(500, testLED);
    
    Serial.print("IP: ");
    Serial.println(instant.getIP());
}

void loop() {
    instant.loop();
    timers.run();
}