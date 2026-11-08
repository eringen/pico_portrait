#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# An existing checkout is only inspected, never reset or updated implicitly.
sdk_dir="${1:-.deps/pico-sdk}"
if [ ! -e "$sdk_dir" ]; then
    git clone --branch 2.2.0 --depth 1 https://github.com/raspberrypi/pico-sdk.git "$sdk_dir"
    git -C "$sdk_dir" submodule update --init lib/tinyusb lib/cyw43-driver lib/lwip
fi
python3 scripts/check_sdk.py "$sdk_dir"
