# Research and next implementation gates

## References inspected

- [py_silhouette device.py](https://github.com/mossblaser/py_silhouette/blob/master/py_silhouette/device.py):
  original Portrait candidate `0x0B4D:0x1123`; `get_state()` sends ESC/ENQ
  (`1b 05`); `get_name()` uses `FG` with ETX. Its constructor resets/configures
  USB and sends a query, so it is not a descriptor-only probe. Its fixed endpoint
  addresses are not evidence of the endpoints on this user's device. `flush()`
  drains its send buffer; that does not establish physical job completion.
- [Inkscape Silhouette Graphtec.py](https://github.com/fablabnbg/inkscape-silhouette/blob/main/silhouette/Graphtec.py):
  the same original Portrait candidate; status-response mapping and command
  framing are useful cross-checks. Model families have different setup behavior.
  This is GPL-2.0 code; no driver implementation was copied into this MIT project.
- [Pico SDK 2.2.0](https://github.com/raspberrypi/pico-sdk/tree/2.2.0):
  board configuration, UART, I²C and CYW43 polling/lwIP integration.
- [Pico examples](https://github.com/raspberrypi/pico-examples):
  local USB-host CMake example, Wi-Fi polling configuration and SSD1306 example
  reviewed. The application implementation and compact OLED font here are new.
- [TinyUSB 0.18.0 host core](https://github.com/hathach/tinyusb/blob/0.18.0/src/host/usbh.c)
  and [device_info example](https://github.com/hathach/tinyusb/tree/0.18.0/examples/host/device_info):
  native RP2040 host enumeration and asynchronous descriptor APIs. Device mount
  occurs after standard SET_CONFIGURATION (configuration value 1 in this pinned
  host stack), even without a matching class driver. The application reads
  configuration index 0 and reports its actual value and endpoints. A device
  with a different first configuration value may fail to mount and needs review.
- [Pico W datasheet](https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf):
  powerchain, VBUS/VSYS distinction and host power requirement. See hardware.md.

Protocol facts above come from software references, **not a USB capture or
successful query on this device**. None of those query bytes is transmitted by
this diagnostic. Mutable upstream driver references were inspected on 2026-11-08;
recheck and pin the exact source revision before implementing protocol support.
The SDK and its linked dependencies are already pinned in the build.

## Stage 2 acceptance: no-motion enumeration

First finish OLED/Wi-Fi validation and the external VBUS measurement gate.
Then obtain the actual VID/PID, complete device/configuration descriptors and
connection behavior. Repeat unplug/replug, wrong-device and power-cycle cases.
A failed enumeration, mismatched ID, multiple ambiguous interfaces or unexpected
endpoints must stop protocol work for investigation. Keep controller and motor
drivers unchanged.

The C diagnostic uses only standard USB enumeration and descriptor reads. It
never opens or submits transfers to the endpoints listed in the configuration.
It does not imply the plotter cannot perform its own power-on behavior.

## Stage 3: status queries and bounded buffering

After descriptor review, select the actual interface/alternate setting and
bulk IN/OUT descriptors by their attributes and the verified device profile.
Implement a narrow allowlist of documented status queries, with finite USB
transfer timeouts, reply-length limits and exact response validation. Log raw
query/reply bytes and compare against both references. Do not add initialization,
homing or pen/motor commands as a side effect of connection or GET status.

Only after status behavior is verified, implement a fixed-size command ring
buffer, explicit job byte/coordinate limits and backpressure. Define rejection
without partial enqueue, transfer ownership and disconnect cleanup. Distinguish
bytes accepted, bytes acknowledged over USB and actual device status. Treat
short writes, unknown replies and cancellation as explicit states. Do not
replay a partly transmitted job after reconnect. Test those transitions on the
host before enabling any plotting route.

## Stage 4: tiny pen plot

Prepare a reviewed preview of a small bounded shape, known origin/units, suitable
pen settings and a stopping procedure. Use a pen, a secured sheet/mat and clear
travel area. Confirm the actual device profile and query behavior first.
The test must be an explicit job submission, never an automatic boot test.
Its exact command sequence is deliberately not invented in this milestone.
Observe physical motion and the final ready state; USB transfer completion alone
must not mark the job physically complete.

## Stage 5: SVG, settings and desktop access

Browser-side SVG parsing should normalize units/viewBox/transforms, flatten
curves with bounded error, preview toolpaths and reject unsupported elements.
Impose geometry/segment/coordinate limits before upload. The Pico should validate
bounded prepared jobs against the device profile through one documented local
API also used by desktop clients. Settings must state their units and supported
range; unknown commands and arbitrary raw USB payloads should not be an API.
No automatic GRBL/G-code compatibility is assumed.
