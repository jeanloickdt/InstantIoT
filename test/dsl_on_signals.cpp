#include "core/SignalEvents.hpp"
#include "utils/InstantIoTWhen.hpp"
#include <cstdio>
static int failures = 0;
#define CHECK(c, q) do { if(!(c)) { printf("ECHEC: %s\n", q); failures++; } } while(0)

// ── What a sketch writes now ──────────────────────────────────
static float jx = 9, jy = 9;
IJoystick(I2, float x, float y) { jx = x; jy = y; }

static bool press = false, released = false, longPress = false;
ISimpleButton(I0) {
    WHEN_PRESSED      { press = true; }
    WHEN_RELEASED     { released = true; }
    WHEN_LONG_PRESSED { longPress = true; }
}

static bool up = false, hautLong = false, basRelache = false;
static bool down = false, left = false, right = false, center = false;
static bool anyRelease = false;
static int  keySeen = -1, keyLong = -1, keyReleased = -1;
IDirectionPad(I3) {
    WHEN_UP           { up = true; }
    WHEN_DOWN         { down = true; }
    WHEN_LEFT         { left = true; }
    WHEN_RIGHT        { right = true; }
    WHEN_CENTER       { center = true; }
    WHEN_UP_LONG      { hautLong = true; }
    WHEN_DOWN_RELEASE { basRelache = true; }
    WHEN_RELEASED_ANY { anyRelease = true; }
    WHEN_PAD_PRESSED(t)      { keySeen = (int)t; }
    WHEN_PAD_LONG_PRESSED(t) { keyLong = (int)t; }
    WHEN_PAD_RELEASED(t)     { keyReleased = (int)t; }
}

static bool press2 = false, release2 = false, long2 = false;
IAdvancedButton(I4) {
    WHEN_PRESSED      { press2 = true; }
    WHEN_RELEASED     { release2 = true; }
    WHEN_LONG_PRESSED { long2 = true; }
}

static bool stop = false, rearmed = false;
IEmergencyButton(I6) {
    WHEN_TRIGGERED { stop = true; }
    WHEN_RESET     { rearmed = true; }
}

static float sliderH = -1, sliderV = -1;
IHorizontalSlider(I7, float v) { sliderH = v; }
IVerticalSlider(I8, float v)   { sliderV = v; }

static int segment = -1;
ISegmentedSwitch(I10, int index) { segment = index; }

static bool lamp = false;
ISwitch(I1, bool on) { lamp = on; }

static float setpoint = 0;
ISignal(I5, float v) { setpoint = v; }

// ── The bench ─────────────────────────────────────────────────
static void send(uint8_t addr, float v) {
    iiot::SignalEvent e; e.address = addr;
    e.value.tag = iiot::SIGNAL_TAG_FLOAT;
    e.value.number = v; e.value.integer = (long)v; e.value.flag = (v != 0.0f);
    iiot::dispatchSignal(e);
}
static void sendText(uint8_t addr, const char* t) {
    iiot::SignalEvent e; e.address = addr;
    e.value.tag = iiot::SIGNAL_TAG_STRING; e.value.string = t;
    iiot::dispatchSignal(e);
}

int main() {
    sendText(2, "0.42,-0.15");
    CHECK(jx > 0.41f && jx < 0.43f, "the joystick receives x");
    CHECK(jy < -0.14f && jy > -0.16f, "the joystick receives y");

    send(0, 1.0f); CHECK(press, "1 = press");
    send(0, 0.0f); CHECK(released, "0 = release");
    send(0, 2.0f); CHECK(longPress, "2 = long press");

    sendText(3, "UP");           CHECK(up, "UP");
    sendText(3, "DOWN");         CHECK(down, "DOWN");
    sendText(3, "LEFT");         CHECK(left, "LEFT");
    sendText(3, "RIGHT");        CHECK(right, "RIGHT");
    sendText(3, "CENTER");       CHECK(center, "CENTER");
    sendText(3, "UP_LONG");      CHECK(hautLong, "UP_LONG");
    sendText(3, "DOWN_RELEASE"); CHECK(basRelache, "DOWN_RELEASE");
    CHECK(anyRelease, "WHEN_RELEASED_ANY takes any release");
    sendText(3, "A");            CHECK(keySeen == (int)iiot::DPadButton::A,
                                            "WHEN_PAD_PRESSED hands back the key");
    sendText(3, "B_LONG");       CHECK(keyLong == (int)iiot::DPadButton::B,
                                            "WHEN_PAD_LONG_PRESSED hands back the key");
    sendText(3, "LEFT_RELEASE"); CHECK(keyReleased == (int)iiot::DPadButton::Left,
                                            "WHEN_PAD_RELEASED hands back the key");

    send(4, 1.0f); CHECK(press2,   "IAdvancedButton: press");
    send(4, 0.0f); CHECK(release2, "IAdvancedButton: release");
    send(4, 2.0f); CHECK(long2,    "IAdvancedButton: long press");

    send(6, 1.0f); CHECK(stop,  "WHEN_TRIGGERED");
    send(6, 0.0f); CHECK(rearmed, "WHEN_RESET");

    send(7, 42.0f); CHECK(sliderH > 41.9f && sliderH < 42.1f, "horizontal slider");
    send(8, 17.0f); CHECK(sliderV > 16.9f && sliderV < 17.1f, "vertical slider");

    send(10, 2.0f); CHECK(segment == 2, "segmented switch");

    send(1, 1.0f); CHECK(lamp, "the switch turns on");
    send(1, 0.0f); CHECK(!lamp, "and off");

    send(5, 21.5f); CHECK(setpoint > 21.4f && setpoint < 21.6f, "ISignal receives its setpoint");

    // An address with no block breaks nothing.
    send(20, 1.0f);

    // ── A RESTORE does not wake a gesture ──
    press = false; setpoint = 0;
    iiot::SignalEvent r; r.address = 0;
    r.value.tag = iiot::SIGNAL_TAG_FLOAT; r.value.number = 1.0f;
    iiot::dispatchSignal(r, /* restore */ true);
    CHECK(!press, "a restore does not fire ISimpleButton");

    iiot::SignalEvent r5; r5.address = 5;
    r5.value.tag = iiot::SIGNAL_TAG_FLOAT; r5.value.number = 19.0f;
    iiot::dispatchSignal(r5, /* restore */ true);
    CHECK(setpoint > 18.9f && setpoint < 19.1f, "but it gives ISignal its setpoint back");

    printf(failures ? "%d FAILURE(S)\n" : "all pass\n", failures);
    return failures;
}
