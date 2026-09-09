/**
 * The single instance of the facade.
 *
 * It lives in a translation unit of its own so there is exactly one, and
 * without a constructor so it is initialised **at compile time** — an
 * `InstantIoT.write(...)` that escapes into another file's global
 * constructor then finds a valid object, not a field not yet written.
 * That is the one static-initialisation-order trap, and it is closed
 * here.
 */
#include "InstantIoT.h"

iiot::Facade InstantIoT;

/**
 * The default for `onSignalWritten` — do nothing.
 *
 * It used to live in `Registry.cpp`, next to nine other defaults written
 * for the old model: `onJoystickEvent`, `onSegmentedSwitchEvent`… That
 * file had not compiled for a long time — it included a `Registry.hpp`
 * that does not exist — and survived only by never being built. This line
 * was the only one still called.
 *
 * Weak: a sketch that defines its own replaces it, with nothing to
 * declare.
 */
__attribute__((weak)) void onSignalWritten(const SignalEvent& e) { (void)e; }
