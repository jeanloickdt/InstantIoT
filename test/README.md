# A bench, for want of a bench

The library has no test framework: it is verified by flashing a board,
which costs time and does not repeat.

These files compile with a plain `g++`, stubbing `Arduino.h` with a few
lines. They do not replace a run on real hardware — they catch what does
not need hardware to be wrong.

```sh
./test/host/run.sh
```

| file | what it pins |
|---|---|
| `host/test_signals.cpp` | the golden frames, byte for byte, through the real `processFrame` |
| `host/test_singleton.cpp` | calling before `begin()`, writing from inside a block, a second `begin()`, an unreadable frame |
| `host/test_destinations.cpp` | the defaults a sketch gets without asking |
| `decoders.cpp` | the gesture convention, positions, pad names |
| `dsl_on_signals.cpp` | that every macro expands, registers and dispatches |

## What they caught by being written

- `e.text()` where `text()` belongs to `SignalValue`, not to `SignalEvent`
- `ISignal` registered as a GESTURE block, because its registration reads
  exactly like the other five — so a restore no longer handed it back its
  setpoint
- a `write` before `begin()` segfaulting: on a board, a reboot
- `dispatchSignalFrame` delivering ONLY restores, which had left every
  `ISignal` and every `ISimpleButton` on the board silent
- a frame the board cannot read vanishing without a word

All of them compiled. No review would have seen them.

Compiling for real needs `arduino-cli`:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/controls/DirectionPad
```
