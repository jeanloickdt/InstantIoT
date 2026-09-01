#include "core/SignalToWidget.hpp"
using namespace iiot;
static int failures = 0;
#define CHECK(cond, quoi) do { if(!(cond)) { printf("ECHEC: %s\n", quoi); failures++; } } while(0)

int main() {
    // ── the gesture convention, as the app writes it ──
    CHECK(gestureFromValue(1.0f) == ButtonEventKind::Press,     "1 = press");
    CHECK(gestureFromValue(0.0f) == ButtonEventKind::Release,   "0 = release");
    CHECK(gestureFromValue(2.0f) == ButtonEventKind::LongPress, "2 = long press");

    // ── a position, to the hundredth as the app writes it ──
    float x = 9, y = 9;
    CHECK(decodePosition("0.42,-0.15", x, y), "position read");
    CHECK(x > 0.41f && x < 0.43f, "x");
    CHECK(y < -0.14f && y > -0.16f, "y");
    CHECK(decodePosition("0.0,0.0", x, y) && x == 0 && y == 0, "retour au center");
    CHECK(!decodePosition("0.42", x, y), "no comma: refused");
    CHECK(!decodePosition("0.42,", x, y), "comma with no second number: refused");
    CHECK(!decodePosition(nullptr, x, y), "null: refused");

    // ── a pad key, gesture included ──
    DPadButton t; DPadEventKind g;
    CHECK(decodePad("UP", t, g) && t == DPadButton::Up && g == DPadEventKind::Press, "UP");
    CHECK(decodePad("UP_LONG", t, g) && t == DPadButton::Up && g == DPadEventKind::LongPress, "UP_LONG");
    CHECK(decodePad("UP_RELEASE", t, g) && t == DPadButton::Up && g == DPadEventKind::Release, "UP_RELEASE");
    CHECK(decodePad("CENTER_RELEASE", t, g) && t == DPadButton::Center && g == DPadEventKind::Release, "CENTER_RELEASE");
    // the app's thirteen keys, including the eight the library ignored
    CHECK(decodePad("A", t, g) && t == DPadButton::A, "A");
    CHECK(decodePad("TRIANGLE_LONG", t, g) && t == DPadButton::Triangle && g == DPadEventKind::LongPress, "TRIANGLE_LONG");
    CHECK(decodePad("CROSS", t, g) && t == DPadButton::Cross, "CROSS");
    CHECK(!decodePad("HAUT", t, g), "unknown word: refused");
    CHECK(!decodePad("", t, g), "empty: refused");

    printf(failures ? "%d FAILURE(S)\n" : "all pass\n", failures);
    return failures;
}
