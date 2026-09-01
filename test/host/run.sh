#!/usr/bin/env bash
# Le banc. Pas de carte, pas d'IDE, environ une seconde.
#
# Il compile avec un `g++` ordinaire, en bouchant `Arduino.h` par les
# quelques lignes d'a cote. Il ne remplace pas un essai sur le materiel : il
# attrape ce qui n'a pas besoin de materiel pour etre faux.
set -e
cd "$(dirname "$0")"

CXX_FLAGS="-std=c++17 -DINSTANTIOT_DEBUG=1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function"
LIB="../../src/InstantIoT.cpp"

# ── Ce qui parle a la lib comme un croquis : depuis test/host, avec Arduino.h
for t in test_signals test_singleton; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I. -o "/tmp/instantiot-$t" "$t.cpp" "$LIB"
    "/tmp/instantiot-$t"
done

# ── Ce qui n'a besoin que des en-tetes : depuis test/, sans Arduino.h
cd ..
for t in decodeurs dsl_sur_signaux; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I../src -Ihost -o "/tmp/instantiot-$t" "$t.cpp"
    "/tmp/instantiot-$t"
done

# ── Ce qui reste dehors
#
# `test_events.cpp` ne compile plus, et le reparer n'est pas une reparation :
# il affirme `typeAtAddress(5) == TYPE_SIMPLEBUTTON` et `ISimpleButton("btn1")`,
# deux choses que la bascule du DSL sur les signaux a retirees exprès. Une
# adresse ne porte plus de type, et un bloc ne s'adresse plus par un nom.
# Le remettre au vert voudrait dire redecider ce qu'un EVENT devient sur la
# carte — un travail, pas une retouche.
echo
echo "hors banc : test_events.cpp (ecrit contre le modele d'avant les signaux)"
