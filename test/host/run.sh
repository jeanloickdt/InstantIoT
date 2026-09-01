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
for t in test_signals test_singleton test_destinations; do
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

