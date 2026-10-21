#!/bin/sh
# Install on the Wii's SD card so it launches from the Homebrew Channel menu:
#   ./install.sh <wii-ip>      (the Wii sitting in HBC)
# Builds a release firmware (no debug log stream, whatever WII_DEBUG_HOST
# says), stages it as hbc/boot.dol next to meta.xml and icon.png, builds
# tools/installer around those three files and sends it through the wiiload
# relay. The installer leaves config.txt, prefs/ and maps/ on the card alone.
set -e
cd "$(dirname "$0")"
WII=${1:?usage: $0 <wii-ip>}
WII_DEBUG_HOST= ./build.sh
if grep -aq "debuglog: streaming" build/meshtasticd.dol; then
    # PlatformIO kept objects from a WII_DEBUG_HOST build: rebuild from clean
    rm -rf third_party/meshtastic-firmware/.pio/build/native-wii
    WII_DEBUG_HOST= ./build.sh
fi
if grep -aq "debuglog: streaming" build/meshtasticd.dol; then
    echo "release build still has the debug log stream" >&2
    exit 1
fi
cp build/meshtasticd.dol hbc/boot.dol
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD":/w -w /w/tools/installer devkitpro/devkitppc:20260503 \
    make -B
DOL=tools/installer/build/meshtasticd-installer.dol ./send.sh "$WII"
