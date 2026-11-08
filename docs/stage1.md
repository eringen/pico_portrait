# Portrait Pico — Stage 1

Hardware diagnostic for Ilhan's Raspberry Pi Pico W and narrow JMD0.91A OLED.
This is a runnable MicroPython test, NOT the final plotting firmware. No USB-host
or motor commands are included. USB enumeration and plotting require later C/C++
firmware with TinyUSB and the Portrait protocol.

## Wiring (power disconnected)

| OLED label | Pico signal | Physical pin |
|---|---|---|
| GND | GND | 3 |
| VCC | 3V3 OUT | 36 |
| SDA | GP0 / I2C0 SDA | 1 |
| SCL | GP1 / I2C0 SCL | 2 |

Use labels, not header position. These settings assume the narrow 128x32 display.
The SSD1306 controller is provisional until the display test succeeds.
Do not connect the Portrait or its internal power supply yet. Power the Pico
only through its normal USB connection to your computer for this test.

## Install and run

1. Back up any existing Pico files before flashing firmware.
2. Download the current stable **RPI_PICO_W** MicroPython UF2 (not Pico 2 W or non-W Pico):
   https://micropython.org/download/RPI_PICO_W/
3. Hold BOOTSEL while plugging the Pico into your computer. Copy the UF2 to
   the RPI-RP2 drive. It will restart.
4. Open Thonny and select its MicroPython Raspberry Pi Pico interpreter and USB port.
5. Copy `config.py` and `main.py` to the root of the Pico filesystem using Thonny.
   The ZIP itself does not go onto the Pico. No additional Python libraries are needed.
6. Initially leave Wi-Fi fields blank. Run main.py. Expect an I2C address of
   0x3C or 0x3D, a border touching all four edges for three seconds, then four
   readable text rows for three seconds, followed by a request to set Wi-Fi.
7. Edit config.py locally with your 2.4 GHz Wi-Fi SSID/password, save to Pico,
   and restart. Do not send your password to ChatGPT.
8. Open `http://<OLED IP address>/` from a device on the same local network.
   `/api/status` returns diagnostic JSON. No internet or cloud service is required.

If main.py starts automatically, stop it with Thonny's Stop button / Ctrl+C to
edit files. If Wi-Fi fails, check credentials and restart; there is deliberately
no automatic reconnect loop in this first diagnostic.

## Expected report

Send the console's I2C address list, whether all four OLED rows render correctly,
and whether the status page opens. Redact credentials. A responding I2C address
does not prove the panel controller; scrambled output needs investigation.

The source files are still `main.py` and `config.py` at the project root.
See [build.md](build.md) for the separate C firmware.

## Next milestones

1. Validate this OLED/Wi-Fi test on physical hardware.
2. Build C/C++ USB-host diagnostic: enumerate VID/PID and descriptors, no motion.
3. Implement Portrait status exchange and bounded command buffering.
4. Test a tiny pen plot, then add SVG conversion, browser UI and local API.
5. Integrate internal power only after measuring rails and spare supply capacity.

The final host connection needs a deliberate regulated 5 V VBUS arrangement.
VSYS power alone does not make the Pico USB connector supply 5 V. Never join
computer VBUS and an internal supply directly, or connect two USB hosts to the
Portrait. Internal mounting/wiring is not specified in this starter package.

## Validation and references

Python syntax checked on the development host. Not run on a physical Pico;
Wi-Fi, panel controller and wiring still require the above hardware test.

https://www.raspberrypi.com/documentation/microcontrollers/micropython.html
https://datasheets.raspberrypi.com/picow/PicoW-A4-Pinout.pdf
https://docs.micropython.org/en/latest/rp2/quickref.html
https://github.com/fablabnbg/inkscape-silhouette

Original starter code supplied under the MIT license; see LICENSE.
