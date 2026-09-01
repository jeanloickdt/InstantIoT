#include "core/SignalToWidget.hpp"
using namespace iiot;
static int echecs = 0;
#define VERIFIE(cond, quoi) do { if(!(cond)) { printf("ECHEC: %s\n", quoi); echecs++; } } while(0)

int main() {
    // ── la convention des gestes, telle que l'app l'ecrit ──
    VERIFIE(gestureFromValue(1.0f) == ButtonEventKind::Press,     "1 = appui");
    VERIFIE(gestureFromValue(0.0f) == ButtonEventKind::Release,   "0 = relachement");
    VERIFIE(gestureFromValue(2.0f) == ButtonEventKind::LongPress, "2 = appui long");

    // ── une position, au centieme comme l'app l'ecrit ──
    float x = 9, y = 9;
    VERIFIE(decodePosition("0.42,-0.15", x, y), "position lue");
    VERIFIE(x > 0.41f && x < 0.43f, "x");
    VERIFIE(y < -0.14f && y > -0.16f, "y");
    VERIFIE(decodePosition("0.0,0.0", x, y) && x == 0 && y == 0, "retour au centre");
    VERIFIE(!decodePosition("0.42", x, y), "sans virgule : refuse");
    VERIFIE(!decodePosition("0.42,", x, y), "virgule sans second nombre : refuse");
    VERIFIE(!decodePosition(nullptr, x, y), "nul : refuse");

    // ── une touche de croix, geste compris ──
    DPadButton t; DPadEventKind g;
    VERIFIE(decodePad("UP", t, g) && t == DPadButton::Up && g == DPadEventKind::Press, "UP");
    VERIFIE(decodePad("UP_LONG", t, g) && t == DPadButton::Up && g == DPadEventKind::LongPress, "UP_LONG");
    VERIFIE(decodePad("UP_RELEASE", t, g) && t == DPadButton::Up && g == DPadEventKind::Release, "UP_RELEASE");
    VERIFIE(decodePad("CENTER_RELEASE", t, g) && t == DPadButton::Center && g == DPadEventKind::Release, "CENTER_RELEASE");
    // les treize touches de l'app, dont les huit que la lib ignorait
    VERIFIE(decodePad("A", t, g) && t == DPadButton::A, "A");
    VERIFIE(decodePad("TRIANGLE_LONG", t, g) && t == DPadButton::Triangle && g == DPadEventKind::LongPress, "TRIANGLE_LONG");
    VERIFIE(decodePad("CROSS", t, g) && t == DPadButton::Cross, "CROSS");
    VERIFIE(!decodePad("HAUT", t, g), "mot inconnu : refuse");
    VERIFIE(!decodePad("", t, g), "vide : refuse");

    printf(echecs ? "%d ECHEC(S)\n" : "tout passe\n", echecs);
    return echecs;
}
