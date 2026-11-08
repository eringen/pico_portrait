# Validation record — 2026-11-08

## Existing project

Inspected `main.py`, redacted configuration assignments, README and LICENSE
before editing. The starter consisted of those four files, with no `.git` or
applicable AGENTS.md present. `config.py` and LICENSE were not modified. Original
README instructions are retained in `docs/stage1.md`; `main.py` gained a real
border display test and a conventional main guard.

The user explicitly reported **nothing has been physically tested yet**.

## Software checks

- CMake configuration checks SDK 2.2.0 and three required submodule commit IDs,
  rejects tracked dependency changes and alternate dependency paths.
- ARM Release builds produce `.elf`, `.bin`, `.uf2` and map/disassembly for
  RP2040 `pico_w`, with USB host startup OFF and ON in separate build directories.
- A third build uses fictitious credentials from a temporary header to compile
  and link the Wi-Fi connection path. No actual Wi-Fi credentials were read into
  the C builds and no firmware was flashed.
- `python3 scripts/test_host.py` checks Python syntax and compiles/runs native C
  tests with `-Wall -Wextra -Werror -fsanitize=address,undefined`.
- Descriptor checks cover short/malformed headers, zero/oversize descriptor
  lengths, exact total length, maximum 512-byte capacity and arbitrary observed
  endpoint values. These protect the application's parser, not TinyUSB internals.
- Mock USB tests cover attach/mount/detach, candidate identification, descriptor
  sequencing, submission errors, incomplete responses, oversize configuration,
  enumeration/descriptor timeouts, ignored late/stale callbacks and disconnect
  counter behavior. The mocks do not emulate a real USB controller or plotter.
- HTTP tests cover exact routes, invalid protocol, non-GET rejection, unknown
  paths, malformed/oversize requests, output bounds, content length, valid JSON,
  zero command bytes and unknown plotting completion.
- Desktop capture decoder checked with synthetic endpoint data and every
  truncated prefix. Ignore rules checked using a temporary git metadata directory;
  no repository was initialized here.

Tests do **not** run on an RP2040 and do not verify TCP delivery, timing, RF,
I²C signaling, TinyUSB enumeration interoperability or power behavior.
The raw lwIP server was compiled, not exercised against a physical network.

## Successful build outputs

All three final Release builds completed without compiler warnings using Arm GCC
14.3.1, CMake 4.1.1 and picotool 2.2.0. Sizes below are GNU `size` text/BSS bytes;
UF2 file size includes transport framing. These are compiled, unflashed artifacts.

| Directory | Configuration | Text bytes | BSS bytes |
|---|---|---:|---:|
| `build/` | Host OFF, no Wi-Fi credentials | 78,760 | 62,652 |
| `build-host/` | Host ON, no Wi-Fi credentials | 81,096 | 62,856 |
| `build-wifi/` | Host ON, fictitious Wi-Fi credentials | 345,472 | 62,924 |

The Wi-Fi compile check includes the radio firmware and connection code that an
empty SSID lets the compiler omit. `build-wifi` is a compile test, not a usable
network configuration. Its temporary config directory is recorded in CMakeCache;
use your own local config for future builds.

## Resource bounds and known limits

One directly attached USB device; no hub/class drivers. 512-byte enumeration
and descriptor buffers; two fixed HTTP client slots; 1024-byte request headers;
4096-byte response storage per client; ten-second absolute client lifetime.
The underlying lwIP pools are also statically configured. No job queue exists.

The main stack is 4096 bytes, within RP2040 SCRATCH_Y. The HTTP body scratch
buffer is static (single-core, non-reentrant use). GCC stack-usage output reports
1208 bytes for JSON formatting, the largest application frame. This is not a
measured runtime stack high-water mark. OLED I²C writes are bounded but may pause
the polling loop for approximately 47 ms per 128x32 refresh (once per second),
longer with 64 rows; timing/interoperability still needs hardware evaluation.

Only configuration index 0 is captured. The pinned TinyUSB stack selects
configuration value 1 during enumeration. Unknown/unsupported devices, malformed
stack-level descriptors or descriptors larger than the enumeration buffer can
fail before the application mount callback. Timeout states require replug/reboot.
No automatic reconnect loop or automatic job replay is implemented.

The local picotool 2.2.0 can generate UF2 but was built without USB support;
it was not used to communicate with hardware. Host-enabled BOOTSEL loading
without reboot requires a USB-enabled picotool; SWD is the preferred route.

## Still required

| Observation | Status |
|---|---|
| OLED ACK address, panel geometry/controller, border/four rows | Not tested |
| Real Wi-Fi join, DHCP IP and HTTP | Not tested |
| UART/SWD on the actual Pico | Not tested |
| External host VBUS circuit and backfeed protection | Proposed, not selected/built/measured |
| Actual Portrait VID/PID, interfaces, endpoints and reconnect | Not tested |
| Portrait status queries/replies | Not implemented; need descriptor evidence first |
| Bounded command queue, tiny pen plot, physical completion | Not implemented |
| SVG submission/settings and desktop job API | Not implemented |
| Internal rails, current allowance, AMS1117 suitability | Unknown; no internal wiring authorized by evidence |

Follow [stage1.md](stage1.md) next with computer USB power and Portrait
disconnected. Proceed to [hardware.md](hardware.md) before any USB-host device
connection. These gates are hardware dependencies, not requests to approve
uncompleted software work.
