/**
 * L'unique exemplaire de la façade.
 *
 * Il vit dans une unité de compilation à lui pour qu'il n'y en ait qu'un,
 * et sans constructeur pour qu'il soit initialisé **à la compilation** —
 * un `InstantIoT.write(...)` échappé dans le constructeur d'une globale
 * d'un autre fichier trouve alors un objet valide, et non un champ pas
 * encore écrit. C'est le seul piège de l'ordre d'initialisation des
 * globales, et il se ferme ici.
 */
#include "InstantIoT.h"

iiot::Facade InstantIoT;

/**
 * Le défaut de `onSignalWritten` — ne rien faire.
 *
 * Il vivait dans `Registry.cpp`, aux côtés de neuf autres défauts écrits
 * pour l'ancien modèle : `onJoystickEvent`, `onSegmentedSwitchEvent`… Ce
 * fichier ne compilait plus depuis longtemps — il incluait un `Registry.hpp`
 * qui n'existe pas — et ne survivait qu'en n'étant jamais bâti. Seule cette
 * ligne-ci était encore appelée.
 *
 * Faible : le croquis qui définit la sienne la remplace, sans rien déclarer.
 */
__attribute__((weak)) void onSignalWritten(const SignalEvent& e) { (void)e; }
