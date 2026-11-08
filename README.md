# Portrait Pico W wireless adapter

An incremental Wi-Fi adapter for the **original Silhouette Portrait**, retaining
its original USB controller and motor drivers. No motor-driver traces are modified.

**Current milestone: compiled no-motion diagnostic, not hardware validated.**
The user has not yet run the OLED/Wi-Fi starter. Plotter status queries, command
buffering, pen plotting, SVG submission and settings are not implemented or enabled.

## Start here

1. Run the [MicroPython OLED/Wi-Fi test](docs/stage1.md) with the Portrait
   disconnected and the Pico powered only by computer USB. The original files
   remain at the project root. `config.py` stays local; `config.example.py` is
   safe to commit. The display now has separate border and four-row tests.
2. Report the scanned I²C addresses, border/row appearance, and whether `/api/status`
   opens. Never include credentials. SSD1306 128x32 remains an assumption.
3. Review [power and debugging](docs/hardware.md). Host VBUS wiring must be defined
   and measured before connecting the Portrait. No internal rail is approved.
4. Follow [reproducible C builds](docs/build.md). USB host startup is **OFF by
   default**. Host-enabled firmware is a separate build for the later bench test.

## What the C firmware contains

- Pico SDK 2.2.0 + its pinned TinyUSB 0.18.0, CYW43 and lwIP dependencies.
- GP0/GP1 I²C OLED probing; four status lines; a missing OLED does not stop USB/Wi-Fi.
- Optional local Wi-Fi credentials and read-only HTTP on port 80.
- Native USB host diagnostic: attach/detach state, VID/PID, raw device and first
  configuration descriptors, UART interface/endpoint reports and bounded reads.
- No USB class drivers, bulk transfers, vendor commands, plotter reset/homing,
  job routes or motion code. Standard USB bus reset/address/configuration are
  part of enumeration; this is not a passive electrical sniffer.
- UART1 on GP4/GP5, 115200 8N1. USB stdio is disabled in both C variants.
- Documented [local API](docs/api.md), shared by browsers and desktop clients;
  `scripts/capture_status.py` retrieves and decodes a descriptor capture using GET.

`0x0B4D:0x1123` is a **reference-derived candidate**, not proof of identity or
compatibility. Endpoints are reported from descriptors, never guessed or used
for commands. Byte transmission will never be treated as proof of plotting completion.

## Checks and next milestones

Run `python3 scripts/test_host.py`. See [validation.md](docs/validation.md) for
compiled artifacts, tested cases, limitations and pending hardware observations.
The [protocol research and staged plan](docs/protocol.md) records what must be
verified before status exchange, bounded jobs and the tiny pen test.

No GRBL/G-code compatibility is promised. Future SVG geometry processing belongs
in the browser; the Pico will validate, buffer and send verified Portrait commands.

This directory was not a git repository when inspected. No repository was
initialized and no commits were made. `.gitignore` excludes local credentials,
build outputs (which may embed credentials), and downloaded SDK dependencies.
Do not force-add those files. Original MIT license retained in `LICENSE`.
