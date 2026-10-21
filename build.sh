#!/bin/sh
# Build the Meshtastic firmware for the Wii:  ./build.sh
#   1. fetch meshtastic/firmware at FW_REF into third_party/meshtastic-firmware
#   2. reset it, apply patches/*.patch, copy overlay/ on top
#   3. build the patched libogc usb.c (libogc-usb/ogc_usb.c)
#   4. pio run -e native-wii in the devkitPPC + PlatformIO container
# Output: build/meshtasticd.dol
# WII_DEBUG_HOST=<ip> also streams the firmware log to <ip>:4302 over Wi-Fi.
set -e
cd "$(dirname "$0")"
ROOT=$PWD
FW=third_party/meshtastic-firmware
FW_REF=v2.8.1.8e6a88d
# MUI (device-ui) at the commit FW_REF pins in platformio.ini [device-ui_base]
DUI=third_party/meshtastic-device-ui
DUI_REF=f77bb709cd83449467520d14344308c63690b624
# libogc sources: only the headers libogc-usb/ogc_usb.c (a patched v3.1.0
# usb.c) needs to compile
LIBOGC=third_party/libogc
LIBOGC_REF=v3.1.0
IMAGE=meshtasticd-wii-build:2
GIT="env GIT_CONFIG_GLOBAL=/dev/null git"

if [ ! -d "$FW/.git" ]; then
    $GIT clone --depth 1 --branch "$FW_REF" https://github.com/meshtastic/firmware "$FW"
    $GIT -C "$FW" submodule update --init --depth 1 protobufs
fi
$GIT -C "$FW" checkout -q -- .
$GIT -C "$FW" clean -qfd -e .pio
for p in patches/*.patch; do
    [ -e "$p" ] && $GIT -C "$FW" apply --whitespace=nowarn "$ROOT/$p"
done
cp -r overlay/. "$FW"/

[ -d "$LIBOGC/.git" ] || $GIT clone -q --depth 1 --branch "$LIBOGC_REF" https://github.com/devkitPro/libogc "$LIBOGC"

# device-ui (MUI): a pinned checkout, our patches and Wii driver on top, then
# staged into the firmware's lib/ so it's built as a project library.
if [ ! -d "$DUI/.git" ]; then
    $GIT init -q "$DUI"
    $GIT -C "$DUI" remote add origin https://github.com/meshtastic/device-ui
    $GIT -C "$DUI" fetch -q --depth 1 origin "$DUI_REF"
fi
$GIT -C "$DUI" checkout -q -f "$DUI_REF"
$GIT -C "$DUI" clean -qfd
for p in patches/device-ui/*.patch; do
    [ -e "$p" ] && $GIT -C "$DUI" apply --whitespace=nowarn "$ROOT/$p"
done
cp -r overlay-device-ui/. "$DUI"/
rm -rf "$FW"/lib/device-ui
mkdir -p "$FW"/lib/device-ui
(cd "$DUI" && tar cf - --exclude=.git --exclude=studio --exclude=docs --exclude=tests --exclude=maps .) | tar xf - -C "$FW"/lib/device-ui

docker image inspect "$IMAGE" >/dev/null 2>&1 || docker build -t "$IMAGE" docker

mkdir -p build
docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT":/w -w /w devkitpro/devkitppc:20260503 \
    /opt/devkitpro/devkitPPC/bin/powerpc-eabi-gcc -O2 -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float -DHW_RVL \
    -Ithird_party/libogc/gc -Ithird_party/libogc/libogc -Ithird_party/libogc/gc/ogc \
    -Ithird_party/libogc/gc/ogc/machine -c -o build/ogc_usb.o libogc-usb/ogc_usb.c

# wolfSSL for MQTT over TLS (WiFiClientSecure), configured by tls/user_settings.h.
# Rebuilt only when the config (or this list) changes.
WOLFSSL=third_party/wolfssl
WOLFSSL_REF=v5.9.4-stable
WOLFSSL_SRC="$(for f in internal keys ssl tls tls13 wolfio; do printf '%s ' "$WOLFSSL/src/$f.c"; done)$(for f in aes asn coding chacha chacha20_poly1305 curve25519 ecc error fe_operations hash hmac kdf logging memory poly1305 random rsa sha sha256 sha512 signature sp_int sp_c32 wc_encrypt wc_port wolfmath md5; do printf '%s ' "$WOLFSSL/wolfcrypt/src/$f.c"; done)"
[ -d "$WOLFSSL/.git" ] || $GIT clone -q --depth 1 --branch "$WOLFSSL_REF" https://github.com/wolfSSL/wolfssl "$WOLFSSL"
if [ ! -f build/libwolfssl.a ] || [ tls/user_settings.h -nt build/libwolfssl.a ] || [ build.sh -nt build/libwolfssl.a ]; then
    docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT":/w -w /w devkitpro/devkitppc:20260503 sh -c '
        set -e; rm -rf build/wolfssl; mkdir -p build/wolfssl
        for f in '"$WOLFSSL_SRC"'; do
            /opt/devkitpro/devkitPPC/bin/powerpc-eabi-gcc -O2 -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float \
                -DWOLFSSL_USER_SETTINGS -Itls -I'"$WOLFSSL"' -c -o build/wolfssl/$(basename $f .c).o $f
        done
        rm -f build/libwolfssl.a
        /opt/devkitpro/devkitPPC/bin/powerpc-eabi-ar rcs build/libwolfssl.a build/wolfssl/*.o'
fi

docker volume create meshtasticd-wii-pio >/dev/null
EXTRA_FLAGS="$WII_EXTRA_FLAGS" # e.g. "-DRADIOLIB_DEBUG_BASIC=1 -DRADIOLIB_DEBUG_SPI=1"
[ -n "$WII_DEBUG_HOST" ] && EXTRA_FLAGS="$EXTRA_FLAGS -DWII_DEBUG_HOST=\\\"$WII_DEBUG_HOST\\\""
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/pio -e PLATFORMIO_CORE_DIR=/pio/.platformio \
    -e PLATFORMIO_BUILD_FLAGS="$EXTRA_FLAGS" \
    -v meshtasticd-wii-pio:/pio -v "$ROOT":/w -w /w/"$FW" "$IMAGE" pio run -e native-wii "$@"

cp "$FW"/.pio/build/native-wii/meshtasticd.dol build/meshtasticd.dol
echo "built build/meshtasticd.dol"
