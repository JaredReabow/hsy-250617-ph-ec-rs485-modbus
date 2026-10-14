# Manuals, documentation and provenance

This repository distinguishes original bench observations from vendor-family
documentation. **Do not combine register maps from different revisions.**
Public availability does not imply an open redistribution license.

## Source inventory

| Source | What was inspected | Relationship to tested unit |
| --- | --- | --- |
| pH+EC Sensor Product Manual V1.5 | Local 10-page PDF; text and relevant rendered pages | Display family match reported in original inspection; exact manufacturer unconfirmed |
| BGT-WMPS(O1) 2023-08-18 manual | Public document transcription, calibration section | Related reference only; older calibration map |
| BGT-WMPS(O1) Series New Ver. | Local 11-page PDF, filename dated 2025-11-04; text and relevant rendered pages | Related reference only; newer calibration map |
| Waveshare ESP32-S3-Relay-1CH wiki/demo | Official adapter reference; demo pin mapping used by bench firmware | Exact RS485 adapter, not the pH-controller manufacturer |
| Modbus application and serial-line specifications | Official standards | Frame/protocol reference, not device register semantics |

### Access links

- [V1.5 manual, public copy](https://www.scribd.com/document/923831164/EC-PH-Meter)
- [BGT 2023-08-18 manual, public copy](https://www.scribd.com/document/763411283/Manual-for-BGT-WMPS-O1-Online-pH-EC-sensor-2023-08-18)
- [BGT website printed in the newer manual](https://www.bjgxhy.com/)
- [Waveshare adapter documentation](https://www.waveshare.com/wiki/ESP32-S3-Relay-1CH)
- [Modbus Application Protocol V1.1b3](https://www.modbus.org/file/secure/modbusprotocolspecification.pdf)
- [Modbus over Serial Line V1.02](https://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf)

## Full-PDF publication status

Selected original product images and the four-wire wiring illustration from
V1.5 are now included at the owner's request, with exact source pages and
third-party-rights attribution in [asset credits](../assets/README.md).
This does not change the full-PDF status below.

The two local PDFs have been inspected but are **not mirrored yet**. Permission
to redistribute them publicly has been requested. No open license was found
in the inspected documents. Source links and original technical notes are
provided meanwhile. There is no verified public download link for the exact
newer PDF; request that revision from the named supplier. Linked third-party
sites may impose their own access conditions.

## V1.5 reference map

- PDF pages 2-3, printed 1-2: advertised specifications.
- PDF page 5, printed 4: wiring and power-on behavior.
- PDF page 6, printed 5: front panel, password 10000 and menus.
- PDF page 7, printed 6: calibration in order 4.00 / 6.86 / 9.18, EC constant.
- PDF page 8, printed 7: address/baud/backlight and 0x0030 address write.
- PDF page 9, printed 8: vendor 0xFE discovery, measurement query and cautions.

No factory-reset or calibration-clear procedure is given in this manual.
Our `0x0037=1` observation is independent bench evidence.

## Calibration-map differences

The **2023 BGT** reference uses 0x006F=0 for pH, followed by 0x0069=1/2/3
for the three buffers, with an approximately eight-second calibration wait.
It describes EC channel 0x006F=1, type 0x005E, constant 0x005F and compensation
0x0054. It is not proven to match the HSY controller. In our old-map tests,
echoes and point readback occurred without successful calibration.

The **newer BGT** local manual, PDF pages 6-8, instead uses:

| Purpose | Newer manual address/value | Tested here? |
| --- | --- | --- |
| Select pH channel | 0x007B=1 | No full calibration sequence tested |
| Select EC channel | 0x007B=2 | No |
| Calibrate pH 4.00 / 6.86 / 9.18 | 0x006A=1 / 2 / 3 | Isolated 0x006A=1 rejected without newer channel selection |
| EC correction/constant | 0x0053, scaled by 1000 | No |

It calls for a power-off/restart after calibration and checking the result.
This is a manual instruction for that product family, not evidence that our
unit's latest calibration or communications settings have survived reboot.

## Documentation inconsistencies worth preserving

- V1.5's example pH/EC response prints CRC 9A A5; the seven-byte payload
  requires **9A 55**. The guide uses the calculated correction, tested offline.
- The newer manual's specifications say 7-24 V DC; one RS485 wiring row says
  5-24 V. Use the actual unit's rating, not the more permissive row.
- The newer pH calibration prose says 6.68 once, while its register table says
  6.86. The three-point set throughout this investigation is 4.00/6.86/9.18.
- Its EC formula text says measured/standard, but the worked example uses
  standard/measured (1413/1200). Do not write EC correction based on this
  contradiction without verifying the required direction on the exact unit.
- Its highest-range EC resolution text disagrees between table and example.
  Confirm the scale using a known standard and retain raw counts.
- Its sample measurement response shows placeholder CRC bytes 00/00. Do not
  copy them into a frame generator; calculate CRC over the actual payload.
- The manuals call 0xFE a broadcast/discovery address; it is vendor-specific,
  not standard Modbus broadcast 0, and only described for a single slave.
- The manual claim that a calibration reply proves success did not hold for
  the older sequence tried on this unit. Trust measurement/outcome validation.

No warranty, accuracy, water-ingress rating or safety certification of related
products is extended to the tested controller by this repository.

## Inspected-file fingerprints

SHA-256 identifies the precise local documents used for this comparison:

```text
V1.5, 10 pages:
82a25817fb39b916837df0217535407e1a63f8e59daf01d23ddbd5aadb40c4d4
New Ver., 11 pages, local filename date 2025-11-04:
6912c340dc6fbf94c539c6d04eba29faf8c8219dcd8ec241a979f11cedb9a9c0
```
