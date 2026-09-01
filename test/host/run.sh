#!/usr/bin/env bash
# The bench. No board, no IDE, about a second.
#
# It compiles with a plain `g++`, stubbing `Arduino.h` with the few lines
# next door. It does not replace a run on real hardware: it catches what
# does not need hardware to be wrong.
set -e
cd "$(dirname "$0")"

CXX_FLAGS="-std=c++17 -DINSTANTIOT_DEBUG=1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function"
LIB="../../src/InstantIoT.cpp"

# ── What talks to the library like a sketch: from test/host, with Arduino.h
for t in test_signals test_singleton test_destinations; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I. -o "/tmp/instantiot-$t" "$t.cpp" "$LIB"
    "/tmp/instantiot-$t"
done

# ── What only needs the headers: from test/, without Arduino.h
cd ..
for t in decoders dsl_on_signals; do
    echo "▸ $t"
    g++ $CXX_FLAGS -I../src -Ihost -o "/tmp/instantiot-$t" "$t.cpp"
    "/tmp/instantiot-$t"
done

