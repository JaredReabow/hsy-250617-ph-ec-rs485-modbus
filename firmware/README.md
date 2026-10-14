# Waveshare ESP32-S3 RS485 bench tool

PlatformIO/Arduino source for **Waveshare ESP32-S3-Relay-1CH**. UART1 TX=17,
RX=18, DE=21. USB CDC is the console at 115200; RS485 is a separate interface.
The relay is unused. Never flash this pin mapping to another board blindly.

## Validation state

- **v1.0.3** ran on the physical adapter for the reset and communications tests.
- Included source **v1.0.4** adds manual UART configuration. It was native-tested
  and firmware-built, but **not flashed or live-tested** in this investigation.
- No controller reset or calibration write is sent on boot. Discovery and
  periodic polling use reads. Raw/calibration writes require explicit commands.
- The source's EC unit label is provisional; EC was zero during tests and the
  manuals disagree on scaling. Do not treat it as validated EC conversion.

## Build and console

From this directory, with PlatformIO installed:

```sh
pio run
pio run -t upload --upload-port YOUR_WAVESHARE_PORT
pio device monitor --port YOUR_WAVESHARE_PORT --baud 115200
```

Identify the adapter port first. Do not flash an unrelated connected ESP32.
The supplied board profile is `esp32-s3-devkitc-1`; build reports an 8 MB flash
profile, while the physical module was described as 16 MB. The small bench
image fits that profile; this is not validation of the entire flash geometry.
The platform URL tracks the pioarduino `stable` archive; the recorded successful
build used platform 55.3.37 / Arduino 3.3.7. Future stable builds may differ.

| Command | Action |
| --- | --- |
| `i` | Status, active ID/baud and counters |
| `p` | Toggle periodic polling; turn off before paced raw captures |
| `r` | Read three base measurement registers |
| `b` | Discover at 9600/4800/19200 and IDs 1-5 |
| `g A C` | Read C=1..8 registers from hexadecimal address A |
| `s` | Read-only sweep 0x0000-0x007F; allow for reply latency |
| `f` | Toggle frame logging |
| `x MS HEX` | One raw PDU, 1..16 bytes, bounded 100..15000 ms capture; CAN WRITE/RESET |
| `u B A F` | v1.0.4: local UART baud, decimal ID, 8N1/8E1/8O1/8N2; disables polling, discovery unverified |
| `c B` | Old-map experimental calibration request, B=400/686/918; NOT a verified calibration method here |

`x 1000 0300000003` reads measurements at the active address, adding CRC.
Raw mode prints timing, bytes, CRC and overflow status and never automatically
retries. Invalid/overlong console input is rejected. The `u` command only
changes the **adapter**, not stored settings in the controller.

See the [controller guide](../docs/controller-guide.md) for address changes
and the [reset warning](../docs/reset-recovery.md) before any raw write.
The v1.0.4 raw builder permits vendor address 254 for investigation; controller
support is not established and it must not be used casually on a shared bus.

## Tests

Run the [combined local harness](../tests/README.md) from the repository root.
It runs the native C++ protocol tests plus documentation/frame/evidence checks.
It never opens a serial port or writes to hardware.
