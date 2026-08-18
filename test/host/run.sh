#!/usr/bin/env bash
# Compiles and runs the host tests. No board, no IDE, about a second.
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -I. \
    -o /tmp/instantiot-test-signals test_signals.cpp ../../src/core/Registry.cpp
/tmp/instantiot-test-signals
