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
#
# Duree : 231 couples — les 21 croquis sur les 11 cartes — soit environ une
# heure a froid, moins ensuite grace au cache d'arduino-cli. C'est long parce
# que c'est exhaustif : chaque croquis d'exemple est compile sur chaque carte,
# et un croquis d'exemple qui ne compile pas est la premiere chose que
# rencontre quelqu'un qui decouvre la bibliotheque.
set -uo pipefail
cd "$(dirname "$0")/.."
RACINE="$(pwd)"

# ── The pairs that are EXPECTED TO FAIL ───────────────────────────────
#
# Format: "FQBN|sketch|reason".
#
# esp8266 / TheCloud WAS here, through stages 3 and 4, waiting on a TLS
# stack. Stage 5 measured BearSSL instead of guessing at it, and the line is
# gone: the ESP8266 reaches the cloud encrypted. That is what this list is
# for — a line leaves it when the code earns it, and never before.
#
# The Mega has no radio at all: AccessPoint and WiFiLink are absent by
# construction, and their shells say so. That is not a gap — it is the board.
#
# EthernetCloud is NOT in this list, and it was — wrongly. It compiles on
# every board here, because Arduino's Ethernet library does. The bench caught
# that: an unexpected GREEN is a surprise, and this one was mine.
ATTENDUS_EN_ECHEC=(
  "arduino:avr:mega|SimpleButton|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|TheCloud|no radio: WiFiLink does not exist on AVR"
  "arduino:avr:mega|OwnServer|no radio: WiFiLink does not exist on AVR"

  # BluetoothClassic — l'ESP32 d'origine, et lui seul. Les S3, C3 et C6
  # ont le BLE sans le Classic ; le S2 n'a aucune radio Bluetooth.
  "esp32:esp32:esp32s3|BluetoothClassic|no Bluetooth Classic radio on this chip"
  "esp32:esp32:esp32c3|BluetoothClassic|no Bluetooth Classic radio on this chip"
  "esp32:esp32:esp32c6|BluetoothClassic|no Bluetooth Classic radio on this chip"
  "esp32:esp32:esp32s2|BluetoothClassic|no Bluetooth Classic radio on this chip"
  "esp8266:esp8266:nodemcuv2|BluetoothClassic|Bluetooth Classic is an ESP32 feature"
  "arduino:renesas_uno:unor4wifi|BluetoothClassic|Bluetooth Classic is an ESP32 feature"
  "arduino:avr:mega|BluetoothClassic|Bluetooth Classic is an ESP32 feature"
  "arduino:samd:mkrwifi1010|BluetoothClassic|Bluetooth Classic is an ESP32 feature"
  "arduino:samd:nano_33_iot|BluetoothClassic|Bluetooth Classic is an ESP32 feature"
  "arduino:megaavr:uno2018|BluetoothClassic|Bluetooth Classic is an ESP32 feature"

  # BluetoothLE — toute la famille ESP32 sauf le S2, qui n'a pas la radio.
  # Ailleurs, NimBLE lui-meme ne se compile pas.
  "esp32:esp32:esp32s2|BluetoothLE|no Bluetooth radio at all on the S2"
  "esp8266:esp8266:nodemcuv2|BluetoothLE|NimBLE is an ESP32 library"
  "arduino:renesas_uno:unor4wifi|BluetoothLE|NimBLE is an ESP32 library"
  "arduino:avr:mega|BluetoothLE|NimBLE is an ESP32 library"
  "arduino:samd:mkrwifi1010|BluetoothLE|NimBLE is an ESP32 library"
  "arduino:samd:nano_33_iot|BluetoothLE|NimBLE is an ESP32 library"
  "arduino:megaavr:uno2018|BluetoothLE|NimBLE is an ESP32 library"

  # SerialModule — AVR et ESP8266. Le Mega et la NodeMCU passent ; les
  # autres coeurs n'embarquent pas SoftwareSerial.
  "esp32:esp32:esp32|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "esp32:esp32:esp32s3|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "esp32:esp32:esp32c3|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "esp32:esp32:esp32c6|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "esp32:esp32:esp32s2|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "arduino:renesas_uno:unor4wifi|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "arduino:samd:mkrwifi1010|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "arduino:samd:nano_33_iot|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"
  "arduino:megaavr:uno2018|SerialModule|SoftwareSerial belongs to the AVR and ESP8266 cores"

  # Les quatorze autres croquis, sur le Mega : douze ouvrent un point
  # d'acces, deux rejoignent un WiFi. Aucune des deux choses n'existe sur
  # une carte sans radio.

  "arduino:avr:mega|Dashboard|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|AdvancedButton|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|DirectionPad|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|EmergencyButton|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|HorizontalSlider|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|Joystick|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|SegmentedSwitch|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|Switch|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|VerticalSlider|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|AnalogTemperature|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|TextAndState|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|UltrasonicLevel|no radio: AccessPoint does not exist on AVR"
  "arduino:avr:mega|Diagnostic|no radio: WiFiLink does not exist on AVR"
  "arduino:avr:mega|Thermostat|no radio: WiFiLink does not exist on AVR"
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
  "arduino:avr:mega"
  "arduino:samd:mkrwifi1010"
  "arduino:samd:nano_33_iot"
  "arduino:megaavr:uno2018"
)

# OwnServer is not decoration: it is the ONLY sketch that takes the
# PLAINTEXT path — `MyServer(...)` without `.secure()` — and therefore the
# only one that compiles TcpClient_ESP32 / TcpClient_R4. TheCloud goes
# through TLS and would have let a broken plain client through unnoticed.
#
# Les trois derniers sont arrives ensemble, et pour la meme raison : leurs
# transports — Bluetooth Classic, BLE, SoftwareSerial — n'etaient compiles
# par RIEN depuis la rupture 2.0. Ils figuraient dans les tableaux avec le
# meme ✅ que le reste, sans qu'aucun compilateur ne les ait vus.
declare -a CROQUIS=(
  "examples/controls/SimpleButton"
  "examples/connection/TheCloud"
  "examples/connection/OwnServer"
  "examples/connection/EthernetCloud"
  "examples/connection/BluetoothClassic"
  "examples/connection/BluetoothLE"
  "examples/connection/SerialModule"

  # Les quatorze autres. Ils n'apprennent rien sur les transports — treize
  # d'entre eux passent par AccessPoint — mais ils sont ce qu'un debutant
  # ouvre en premier, et un croquis d'exemple qui ne compile pas est pire
  # qu'un croquis absent.
  "examples/complete/Dashboard"
  "examples/complete/Diagnostic"
  "examples/complete/Thermostat"
  "examples/controls/AdvancedButton"
  "examples/controls/DirectionPad"
  "examples/controls/EmergencyButton"
  "examples/controls/HorizontalSlider"
  "examples/controls/Joystick"
  "examples/controls/SegmentedSwitch"
  "examples/controls/Switch"
  "examples/controls/VerticalSlider"
  "examples/measurements/AnalogTemperature"
  "examples/measurements/TextAndState"
  "examples/measurements/UltrasonicLevel"
)

# ── Les options de carte qu'un croquis exige ──────────────────────────
#
# La pile Bluetooth ne tient pas dans la partition applicative par defaut
# d'un ESP32 : 1,31 Mo, et Bluedroid seul en demande 1,60. Le message du
# constructeur est *text section exceeds available space in board*, ce qui
# ressemble a un bug de la bibliotheque et n'en est pas un.
#
# Le choix se fait dans Outils → Partition Scheme, donc dans la FQBN ici.
# Les deux croquis Bluetooth le disent aussi dans leur en-tete : le banc et
# la personne qui televerse doivent faire le meme geste.
options_de() {
    case "$1:$2" in
        esp32:esp32:*:BluetoothClassic|esp32:esp32:*:BluetoothLE)
            echo ":PartitionScheme=huge_app" ;;
        *) echo "" ;;
    esac
}

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

        fqbn_reel="$fqbn$(options_de "$fqbn" "$nom")"

        if arduino-cli compile --fqbn "$fqbn_reel" --libraries "$BIBLIOTHEQUES" \
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
