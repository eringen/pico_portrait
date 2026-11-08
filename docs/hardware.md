# Hardware validation gates

## Now: OLED/Wi-Fi only

Portrait and internal supply disconnected. Pico powered solely by computer USB.
Wire with power removed:

| OLED | Pico signal | Physical pin |
|---|---|---|
| GND | GND | 3 |
| VCC | 3V3 OUT | 36 |
| SDA | GP0 / I²C0 | 1 |
| SCL | GP1 / I²C0 | 2 |

Use the [stage 1 procedure](stage1.md). Record scan addresses, border, four rows,
and HTTP JSON. If there is no ACK, check labels, continuity, ground and 3.3 V
before trying software changes. If there is ACK but scrambled graphics, the
controller/geometry is still unknown. Do not assume success from ACK alone.
Do not pull SDA/SCL up to 5 V. OLED height defaults to 32 and can be set to 64
only if evidence supports that geometry.

## Proposed external host bench power (not yet built or verified)

The Pico W datasheet sections 3.4/3.5 (PDF pages 15-16 in the downloaded revision)
show the VBUS-to-VSYS diode and require 5 V at VBUS for native USB host mode.
VSYS power cannot flow backward through that diode to create USB VBUS.
[Official Pico W datasheet](https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf)

The proposed initial arrangement, subject to component selection and measurements:

```text
regulated external 5 V
        |
 current limiting/fuse + reverse-current protection + physical disconnect
        |
        +--> Pico VBUS, physical pin 40 --> native USB connector VBUS
        |                                   |
        |                              host cable/adaptor --> Portrait USB
        +--> Pico's onboard diode --> VSYS --> onboard 3.3 V regulator
supply ground --> Pico GND and cable ground
```

No extra VSYS feed or VBUS/VSYS short is needed for this proposal. A cable alone
cannot create power. USB signal routing/polarity and adaptor pinout must be
checked; native micro-USB D+/D- remain the data connection, not arbitrary GPIO.
The Portrait keeps its original power supply/controller. Do not assume its USB
VBUS draw is zero because the motors have separate power.

Before **asking for a Portrait connection test**, record:

1. The exact external regulated supply, current limit, protective components,
   cable/adaptor pinout and a wiring diagram. No component has been selected here.
2. With computer and plotter detached: correct polarity and nominal 5 V at Pico
   VBUS and downstream connector, with voltage remaining within USB supply limits
   under the planned load; verify against selected supply/component specifications.
3. Power budget and measured Pico/OLED/radio load, downstream allowance and
   transients. Confirm current limiting protects wiring and a faulted port.
4. Reverse-current isolation or physical removal of every alternative source.
   Check the proposed wiring powered and unpowered, without exposing computer USB
   to an external source. Do not assume an inexpensive hub/adaptor prevents backfeed.
5. Working 3.3 V UART or SWD, no power feed from the UART adapter, and computer USB
   physically absent from the Pico's USB data/power connection during host operation.

Only after that gate: use host firmware for a descriptor-only capture, observe
attach/detach/reconnect, and save `/api/status`. Standard USB enumeration includes
bus reset, SET_ADDRESS and SET_CONFIGURATION. No plotter command is transmitted,
but autonomous device behavior at power-up/enumeration is unverified.

## Internal installation remains deferred

Internal rail voltage, startup/no-load/load behavior, spare current, ground and
thermal environment have not been measured. No internal power tap is specified.
Do not modify motor-driver traces. Keep the Portrait's original control path.

The owned AMS1117-5.0 is not approved for this installation. Its actual maker,
module layout, input rating, dropout, capacitors and thermal limits need checking.
A linear regulator's approximate dissipation is `(Vin - 5 V) × I`; suitability
requires measured input/load and thermal analysis against the actual part's
datasheet. There is no justified assumed internal input voltage yet.
