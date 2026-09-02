#!/usr/bin/env bash
# The board bench. Every (board, sketch) pair, compiled for real.
#
# ## Why a list of EXPECTED FAILURES
#
# A compile matrix that only reports green tells you nothing about the day a
# combination starts working by accident. This script knows what is supposed
# to fail and why — so an unexpected PASS is a failure too, and the exit code
# says it.
#
# That is what makes the list below a work ticket rather than a wart: a stage
# is done when a line comes out of ATTENDUS_EN_ECHEC and the script still
# exits 0.
#
# Usage:  bash test/boards.sh
# Exit :  0 = every result matches expectations · 1 = something surprised us
set -uo pipefail
cd "$(dirname "$0")/.."
RACINE="$(pwd)"

# ── The pairs that are EXPECTED TO FAIL ───────────────────────────────
#
# Format: "FQBN|sketch|reason".
#
# esp8266 / TheCloud and OwnServer — there is no TcpClient_ESP8266 yet. The
# `#elif defined(ARDUINO_ARCH_ESP8266)` block of Links.hpp only defines
# INSTANTIOT_HAS_SERIAL_LINK, so WiFiLink falls through to its absence shell
# and the static_assert fires. Stage 3 removes this line.
ATTENDUS_EN_ECHEC=(
  "esp8266:esp8266:nodemcuv2|TheCloud|no TcpClient_ESP8266 yet (stage 3)"
  "esp8266:esp8266:nodemcuv2|OwnServer|no TcpClient_ESP8266 yet (stage 3)"
)

# ── The matrix ────────────────────────────────────────────────────────
#
# SimpleButton uses AccessPoint: the board IS the network, so it must build
# everywhere that has a radio. TheCloud uses WiFiLink + Cloud, which needs a
# TCP client and a TLS stack.
declare -a CARTES=(
  "esp32:esp32:esp32"
  "esp32:esp32:esp32s3"
  "esp32:esp32:esp32c3"
  "esp32:esp32:esp32c6"
  "esp32:esp32:esp32s2"
  "esp8266:esp8266:nodemcuv2"
  "arduino:renesas_uno:unor4wifi"
)

# OwnServer is not decoration: it is the ONLY sketch that takes the
# PLAINTEXT path — `MyServer(...)` without `.secure()` — and therefore the
# only one that compiles TcpClient_ESP32 / TcpClient_R4. TheCloud goes
# through TLS and would have let a broken plain client through unnoticed.
declare -a CROQUIS=(
  "examples/controls/SimpleButton"
  "examples/connection/TheCloud"
  "examples/connection/OwnServer"
)

# ── The cores must be there. We say so; we do not install them. ───────
#
# Installing a core behind someone's back downloads hundreds of megabytes and
# can change a toolchain version under a build they were debugging.
manquants=()
for fqbn in "${CARTES[@]}"; do
    paquet="${fqbn%%:*}:$(echo "$fqbn" | cut -d: -f2)"
    arduino-cli core list 2>/dev/null | awk 'NR>1 {print $1}' | grep -qx "$paquet" \
        || manquants+=("$paquet")
done
if [ ${#manquants[@]} -gt 0 ]; then
    echo "Missing cores. Install them, then run this again:"
    printf '  arduino-cli core install %s\n' $(printf '%s\n' "${manquants[@]}" | sort -u)
    exit 1
fi

# ── The library must be visible to the compiler ───────────────────────
#
# --libraries points arduino-cli at the parent directory, so it discovers
# THIS checkout rather than whatever is installed in ~/Documents/Arduino.
BIBLIOTHEQUES="$(dirname "$RACINE")"

est_attendu_en_echec() {
    local fqbn="$1" croquis="$2"
    for e in "${ATTENDUS_EN_ECHEC[@]}"; do
        [ "${e%%|*}" = "$fqbn" ] || continue
        local reste="${e#*|}"
        [ "${reste%%|*}" = "$croquis" ] && { echo "${reste#*|}"; return 0; }
    done
    return 1
}

surprises=0
for fqbn in "${CARTES[@]}"; do
    for chemin in "${CROQUIS[@]}"; do
        nom="$(basename "$chemin")"
        raison="$(est_attendu_en_echec "$fqbn" "$nom")" && attendu_echec=1 || attendu_echec=0

        if arduino-cli compile --fqbn "$fqbn" --libraries "$BIBLIOTHEQUES" \
             "$chemin" > /tmp/iiot-board.log 2>&1; then
            if [ "$attendu_echec" = 1 ]; then
                printf '✗ %-30s %-14s SURPRISE: passes, but was expected to fail (%s)\n' \
                       "$fqbn" "$nom" "$raison"
                surprises=$((surprises + 1))
            else
                printf '✓ %-30s %s\n' "$fqbn" "$nom"
            fi
        else
            if [ "$attendu_echec" = 1 ]; then
                printf '· %-30s %-14s (expected failure: %s)\n' "$fqbn" "$nom" "$raison"
            else
                printf '✗ %-30s %-14s FAILED\n' "$fqbn" "$nom"
                sed -n 's/^\(.*error:.*\)$/    \1/p' /tmp/iiot-board.log | head -3
                surprises=$((surprises + 1))
            fi
        fi
    done
done

echo
if [ "$surprises" -eq 0 ]; then
    echo "all as expected"
    exit 0
fi
echo "$surprises surprise(s) — a green where a red was expected counts too"
exit 1
