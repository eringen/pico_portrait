# Using the RP2040-Zero as the debug adapter

The spare is the [Motorobit RP2040-Zero](https://www.motorobit.com/rp2040-zero-raspberry-pi-cipli-mcu-programlama-karti),
identified by the user's product link. Its listed hardware and pinout expose the
GPIOs required for Raspberry Pi debugprobe's UART/SWD implementation.

Use `debugprobe_on_pico.uf2` v2.3.1 as a compatibility test. This is the official
Pico image, not a custom Zero build or a hardware-verified image for this clone.
The [SDK Zero board definition](https://github.com/raspberrypi/pico-sdk/blob/2.2.0/src/boards/include/boards/waveshare_rp2040_zero.h)
uses the same flash boot routine as Pico, though its flash clock divider differs.
The debugprobe release runs from RAM. Compatibility is inferred from these
source details and must be checked by USB enumeration and UART loopback.
The Zero's RGB LED uses GP16; the Pico image's indicator uses GP25, so a dark RGB
LED is not a failure indication. Leave unused GPIOs disconnected.

## Prepare the spare board independently

Leave the project Pico W, OLED and Portrait unchanged. Disconnect every jumper
from the spare and back up any files/program on it that you want to retain.

Download from the official [debugprobe v2.3.1 release](https://github.com/raspberrypi/debugprobe/releases/tag/debugprobe-v2.3.1):

- Use `debugprobe_on_pico.uf2` for this RP2040-Zero test.
- Do not select `debugprobe.uf2` (dedicated probe accessory) or the Pico 2 image.
- A downloaded copy is at `build-debugprobe/debugprobe_on_pico.uf2` (git-ignored).
  Its SHA-256 matches the official release asset:
  `6649ebba11df46cfa6cfa1faec9706bd21303893cf3f1e40067284bd4e3792d4`.
  RP2040 UF2 family ID, block structure and flash addresses were also checked.
  It has not been flashed or physically tested.

Hold the Zero's BOOT button while connecting only the spare to computer USB-C, then copy its
matching image to the bootloader drive. After reboot it should expose a USB
serial interface and CMSIS-DAP debugger. This replaces its previous firmware;
it will no longer run MicroPython. No firmware has been flashed by the assistant.

## Standalone UART loopback first

Disconnect the project Pico W from computer USB to identify the new serial port
unambiguously. Leave its OLED wiring and MicroPython files intact. No external
supply or Portrait is needed for this test.

After flashing the Zero, unplug it. Join the Zero's **GPIO4 to GPIO5** with one
jumper; these are GPIO labels, not Pico header position numbers. Reconnect it to
computer USB-C. On macOS list the port with `ls /dev/cu.usbmodem*`, then run
`screen /dev/cu.usbmodemXXXXX 115200`, replacing the path with the actual result.
Type `hello`: it should return through the physical UART jumper. Local echo must
be off (screen's default). To exit screen press Ctrl-A, then K, then Y.

For a negative check, unplug, remove the jumper, reconnect and reopen screen:
typed characters should no longer echo. A successful paired test verifies the
USB/UART bridge; it does not verify SWD, the target or host VBUS. If no serial port
appears, retry BOOT and report that result instead of connecting the target.

## RP2040-Zero wiring reference (later)

Pin functions verified against [the v2.3.1 Pico board configuration](https://github.com/raspberrypi/debugprobe/blob/debugprobe-v2.3.1/include/board_pico_config.h).
Target UART pins come from this project's CMake configuration, not the SDK's
usual GP0/GP1 defaults; GP0/GP1 on the target remain dedicated to the OLED.

| RP2040-Zero GPIO label | Project Pico W |
|---|---|
| GP5 / RX | GP4 / TX, physical 6 |
| GP4 / TX (optional) | GP5 / RX |
| GND | GND, e.g. physical 8 |
| GP2 / SWCLK | Dedicated SWCLK pad |
| GP3 / SWDIO | Dedicated SWDIO pad |

UART uses 115200 baud, 8N1, no flow control. UART alone needs ground and the
first row to receive logs. SWD connections are separate and enable programming
without using the target USB connector. Do not confuse target SWD pads with its
numbered GPIO header pins.

Connect with both boards unpowered. Do not connect 3V3, VSYS or VBUS between
boards. Computer USB powers the probe. Power the target separately according to
the current test stage. Avoid driving signal pins into an unpowered target.
[Official probe connection guidance](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html)

The MicroPython starter does not emit UART logs on GP4. This UART configuration
is for the C diagnostic; no log output from unchanged stage 1 is expected.
Keep the Portrait disconnected. Preparing a probe does not verify host VBUS.
Before host testing, obtain the external supply label/model/current rating and
complete [the power gate](hardware.md). No internal rail or AMS1117 wiring is
specified by this document.
