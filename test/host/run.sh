#!/usr/bin/env bash
# The bench. No board, no IDE, about a second.
#
# It compiles with a plain `g++`, stubbing `Arduino.h` with the few lines
# next door. It does not replace a run on real hardware: it catches what
# does not need hardware to be wrong.
set -e
cd "$(dirname "$0")"

# `-Werror` : un avertissement qui ne fait pas echouer le banc est un
# avertissement que personne ne lit.
#
# `-Wno-error=#warnings` avec, et ce n'est pas un contournement : le banc
# compile pour l'HOTE, qui n'est aucune plateforme officielle, donc le
# `#warning` de InstantIoTConfig.h se declenche a chaque test. Cet
# avertissement est VOULU — il doit rester visible pour qui compile sur une
# carte exotique — mais il ne dit rien sur le code qu'on teste ici.
CXX_FLAGS="-std=c++17 -DINSTANTIOT_DEBUG=1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Werror -Wno-error=#warnings"
LIB="../../src/InstantIoT.cpp"

# ── What talks to the library like a sketch: from test/host, with Arduino.h
for t in test_signals test_rate test_singleton test_destinations; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I. -o "/tmp/instantiot-$t" "$t.cpp" "$LIB"
    "/tmp/instantiot-$t"
done

# ── What only needs the headers: from test/, without Arduino.h
cd ..
for t in decoders dsl_on_signals heartbeat_frame; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I../src -Ihost -o "/tmp/instantiot-$t" "$t.cpp"
    "/tmp/instantiot-$t"
done

