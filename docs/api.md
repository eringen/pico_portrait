# Local diagnostic API v1

This contract describes **C firmware 0.2.0**, not the existing MicroPython stage 1
JSON. It is plain HTTP on the Pico's DHCP address, port 80. No internet service,
cloud account, Silhouette Studio or desktop driver is required for HTTP access.

| Method / path | Result |
|---|---|
| `GET /` | Minimal diagnostic page, HTTP 200 |
| `GET /api/status` | Current snapshot, JSON, HTTP 200 |
| Other GET path | HTTP 404 |
| Any non-GET method, including POST/OPTIONS | HTTP 405, `Allow: GET` |
| Malformed request | HTTP 400 |
| Headers exceed 1024 bytes | HTTP 431 |

Connections close after one response. No pipelining, uploads, chunked bodies,
CORS permission, authentication or TLS. Use a trusted LAN; do not port-forward.
The browser diagnostic page uses the same origin. Desktop applications can GET
the same endpoint directly; remote browser origins are not an approved job path.
A future write API needs explicit origin/authentication and bounded job rules.

## Snapshot fields

| Field | Meaning |
|---|---|
| `api_version`, `firmware`, `stage` | `1`, `"0.2.0"`, `2` |
| `wifi.state`, `wifi.ip` | Radio/link state and IPv4 address; `0.0.0.0` before a lease or after loss |
| `oled.address` | Numeric I²C address, or 0 if detection/init failed |
| `oled.io_ok` | Last display write succeeded; not visual verification |
| `oled.controller_verified` | Always false in this unverified hardware milestone |
| `usb.host_enabled` | Host controller was initialized; does not assert safe VBUS |
| `usb.connected` | Physical bus attach observed (even if enumeration fails) |
| `usb.mounted` | TinyUSB reached configured state; not Portrait protocol readiness |
| `usb.state` | Diagnostic phase or failure, below |
| `usb.address`, `vid`, `pid` | Numeric USB address and IDs; zero until known/after detach |
| `usb.portrait_candidate` | VID/PID match `0x0B4D:0x1123`; does not imply verified compatibility |
| `usb.mounts`, `disconnects` | Counters since boot; an enumerating detach also counts |
| `usb.device_descriptor_hex` | 18 raw descriptor bytes as lowercase hex, or empty |
| `usb.configuration_index` | 0: only the first configuration is inspected |
| `usb.configuration_descriptor_hex` | Validated descriptor bytes, maximum 512, or empty |
| `capabilities.status_query`, `job_submission`, `svg` | All false |
| `motion_enabled` | Always false |
| `job.state` | `unsupported` |
| `job.command_bytes_sent` | Always 0; excludes USB enumeration traffic |
| `job.plotting_complete` | `null`: no completion evidence, never inferred from transmission |

No SSID/password, USB string descriptors or serial number strings are exposed.
USB string indices remain in the raw device descriptor; strings are not fetched.

Normal host states: `disabled_vbus_unverified`, `disconnected`, `enumerating`,
`reading_device_descriptor`, `reading_configuration_header`,
`reading_configuration`, `descriptors_ready`.
Errors: `host_init_failed`, `descriptor_submit_failed`,
`descriptor_transfer_failed`, `bad_device_descriptor`,
`bad_configuration_header`, `configuration_too_large_or_invalid`,
`bad_configuration_descriptor`, `descriptor_timeout_replug_required`,
`enumeration_timeout_replug_required`.

TinyUSB itself must first enumerate/configure the device. A configuration larger
than its 512-byte enumeration buffer may never mount; the application then
reports `enumeration_timeout_replug_required`, not the post-mount size error.
Enumeration starts a five-second observation timeout; post-mount descriptor
reads have three seconds each. A timeout marks failure and requires unplug/replug
or reboot. The application does not reuse a timed-out transfer buffer while the
same device is attached. A stalled enumeration is not automatically reset.
Only one directly connected device is supported. Hubs are disabled.

## Desktop capture

Once running with a DHCP address:

```sh
python3 scripts/capture_status.py http://192.168.1.123 --output /tmp/portrait-status.json
```

The address is illustrative. The tool uses GET only and refuses to overwrite a
capture. It prints descriptor-derived interfaces/endpoints. Share the JSON and
UART log after checking for any identifying device data you do not want to share.
It does not enable USB, send plotter commands, or run on the Pico itself.

## Future job semantics (not implemented)

The planned API will distinguish acceptance, buffering, transmitting, device
busy, observed completion, cancellation requested, failure and unknown state.
A TCP upload/USB ACK proves transport progress only. On timeout/disconnection,
completion must remain unknown and queued bytes must not automatically replay
on reconnect. No job endpoints or raw-command escape hatch exist in this build.
