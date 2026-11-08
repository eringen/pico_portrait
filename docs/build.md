# Reproducible RP2040 Pico W build

## Inputs

Use CMake >=3.20, Make (or Ninja), Python 3, Git, the Arm GNU embedded toolchain,
and picotool 2.2.0. This project was compiled with CMake 4.1.1 and Arm GCC
14.3.Rel1 (14.3.1). Reproducible here means pinned source inputs and documented
compiler/tools; identical bytes across paths or compiler versions are not claimed.

| Dependency | Required commit |
|---|---|
| Pico SDK 2.2.0 | `a1438dff1d38bd9c65dbd693f0e5db4b9ae91779` |
| TinyUSB 0.18.0 | `86ad6e56c1700e85f1c5678607a762cfe3aa2f47` |
| CYW43 driver | `dd7568229f3bf7a37737b9e1ef250c26efe75b23` |
| lwIP | `77dcd25a72509eb83f72b033d219b1d40cd8eb95` |

CMake checks these revisions and rejects tracked modifications. It does not
silently update an SDK. Use the bundled dependency paths; overriding
`PICO_TINYUSB_PATH`, `PICO_CYW43_DRIVER_PATH` or `PICO_LWIP_PATH` is unsupported.

From the project root, either use an existing clean SDK at those revisions or:

```sh
sh scripts/bootstrap-sdk.sh
export PICO_SDK_PATH="$PWD/.deps/pico-sdk"
```

On this development machine the clean SDK is already at
`/Users/eringen/projects/pico/pico-sdk`. It was inspected read-only, not modified.
The bootstrap script needs network access only when creating its checkout.

## Optional credentials

The C firmware does **not** read MicroPython `config.py`. For C Wi-Fi testing,
copy `firmware/config.example.h` to `firmware/config.local.h` and edit locally.
Do not overwrite an existing local file. Set the same 2.4 GHz network and OLED
pins there. An absent file gives a valid build with Wi-Fi disabled.

Do not place passwords in CMake flags, shell commands, screenshots or reports.
A firmware image with credentials embeds them in flash; do not publish that image.
The default and host diagnostic builds contain empty credentials. A separate
compile check uses explicitly fictitious Wi-Fi credentials to ensure the
connection path is linked, without exposing or using the local `config.py`.
You can keep `config.local.h` outside the project and set
`-DPORTRAIT_CONFIG_DIR=/absolute/private/directory` to use it.

## Default build: USB host disabled

```sh
cmake -S firmware -B build \
  -DPICO_SDK_PATH="$PICO_SDK_PATH" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPORTRAIT_USB_HOST=OFF
cmake --build build --parallel 8
python3 scripts/test_host.py
```

CMake can locate installed picotool 2.2.0 or fetch/build it from its SDK-selected
2.2.0 tag. For an offline build add `-Dpicotool_DIR=/path/to/picotool/package`.
This machine used
`/Users/eringen/projects/pico/joystick/build/_deps/picotool`, which contains
picotool 2.2.0 and its CMake package. This local picotool was built without
USB support: it generates UF2 but cannot load a connected device. The later
no-reboot loading procedure requires a USB-enabled picotool or an SWD probe. No SDK download was required.

Outputs: `build/portrait_diagnostic.uf2`, `.elf`, `.bin`, `.dis`, `.elf.map`.
Only `pico_w` (RP2040) is accepted; Pico 2 W and non-W boards are outside scope.

## Later host-enabled build

**Compilation is permitted now; running this variant requires the hardware gate
in [hardware.md](hardware.md).** Do not flash it during the computer-powered
stage 1 test. On reset it immediately starts the native host controller.

```sh
cmake -S firmware -B build-host \
  -DPICO_SDK_PATH="$PICO_SDK_PATH" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPORTRAIT_USB_HOST=ON
cmake --build build-host --parallel 8
```

The output is `build-host/portrait_diagnostic.uf2`. The host-enable flag does not
supply, switch or measure VBUS; it is a procedural gate, not electrical protection.

## Flashing and debugging

Back up any files already on the Pico before replacing MicroPython. The two
firmware types are alternatives, not programs that run together.

For the **default host-disabled** C build: disconnect every external supply and
USB downstream cable, hold BOOTSEL, attach the Pico to the computer, copy its UF2
to RPI-RP2, and let it restart. There is no USB serial console; use UART or the
OLED and Wi-Fi. This build is not a substitute for reporting stage 1 observations.

For **host-enabled** firmware use SWD with the verified bench power arrangement.
If BOOTSEL loading is needed, detach downstream USB and all external power,
load the UF2 without automatic reboot (`picotool load` without `-x`), then unplug
computer USB before restarting on the verified bench supply. Do not drag-copy
host-enabled UF2 and leave it running attached to a computer host.
Never have computer USB data and Portrait USB data on the Pico simultaneously.

UART1 TX GP4 (physical 6) -> 3.3 V USB-UART receiver RX; common GND, e.g. pin 8.
Optional receiver TX -> GP5 (physical 7). Leave adapter power pins disconnected.
Use 115200 baud, 8N1; no RS-232 levels and no 5 V logic. SWD uses the dedicated
SWD pads/debug connector and a compatible 3.3 V probe; it is independent of USB.
