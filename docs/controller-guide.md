# HSY-250617-v2.3 pH / EC Controller Reference

Independent field notes, 2026-10-14. Read the safety and variant warnings before
writing any register. This is a collected technical reference, not an official
service manual or a claim of manufacturer endorsement.

## 1. Identify the unit first

| Identifier | This investigation |
| --- | --- |
| Front-panel wording | Water Quality Sensor, as reported in the original hardware inspection |
| Rear-label wording | Five-in-One Water Quality Sensor, as reported in the original hardware inspection |
| PCB marking | HSY-250617-v2.3, supplied by the owner |
| Readable firmware string | POET-251022-V4.2 at registers 0x0040-0x0047 |
| Default communications | ID 1, 9600 baud, 8N1; manual and reset readback agree |
| Final bench communications | ID 2, 4800 baud, 8N1; write/readback verified |
| Adapter used | Waveshare ESP32-S3-Relay-1CH with custom bench firmware |

The manufacturer and exact commercial model are not established. BGT-WMPS(O1)
documents describe related products with overlapping protocol details, but are
not proof that this controller is a BGT product. Do not use a generic product
photo or enclosure shape as the sole basis for a write operation.

The name "Five-in-One" does not establish five validated measurement registers.
Only pH was exercised with buffers here; EC and temperature remained zero.

## 2. Electrical connection

![Conceptual bench wiring](../assets/rs485-wiring.svg)

The V1.5 manual and the owner's rear-label inspection give this four-wire map:

| Wire | Function | Bench connection |
| --- | --- | --- |
| Red | Controller DC supply positive | Suitable external controller supply |
| Black | Controller DC supply negative | Controller supply return |
| Yellow | RS485 A+ | Waveshare A+ terminal |
| Green | RS485 B- | Waveshare B- terminal |

**Manual rating:** 7-24 V DC. Verify the actual device label before powering it.
Do not feed this voltage into ESP32 GPIO or connect A/B directly to TTL UART.
The newer BGT manual contradicts itself with 7-24 V in specifications and 5-24 V
in one wiring row; that is not permission to assume 5 V works on this unit.

The Waveshare provides the RS485 transceiver. The bench software uses TX GPIO17,
RX GPIO18, direction GPIO21, UART1 hardware half-duplex. These are **adapter
pins**, not pH-controller pins. The relay is not used for this investigation.
See [Waveshare documentation](https://www.waveshare.com/wiki/ESP32-S3-Relay-1CH)
and the vendor demo's `WS_GPIO.h` for the adapter mapping.

For another adapter, use a suitable RS485 interface and follow its isolation,
reference-ground, biasing and termination instructions. Terminate a bus at its
physical ends as required, not at every node. A/B naming varies between vendors;
verify polarity instead of trusting letters alone. Do not defeat an isolated
interface by casually tying its isolated and USB grounds together.

Disconnect pumps, dosing systems and other controlled outputs before reset or
calibration. Power down before rewiring. Only one master should communicate on
this bench bus. The general serial-bus reference is the
[Modbus Serial Line Guide V1.02](https://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf).

## 3. Manual specifications versus measurements

The local **pH+EC Sensor Product Manual V1.5** claims pH 0-14, EC 0-2000 uS/cm,
temperature 0-60 C, accuracy +/-0.02 pH, +/-1.5% FS EC and +/-0.2 C. It lists
0.01 pH, 0.1 uS/cm and 0.1 C resolution. These are **manual claims**, not
independently measured specifications for this PCB revision.

Its sample EC response decodes integer 404 as 404 uS/cm despite the finer
resolution claim. The newer BGT manual has range-dependent EC divisors. Keep
raw EC counts until the exact variant and reference-solution scaling are known.
The bench firmware's `EC=uS/cm` label is provisional for this unit.

## 4. Front-panel operation and calibration

V1.5 describes entry through the confirm key, default menu password **10000**,
up/down navigation and a return key. Menu functions include pH calibration,
EC electrode constant, communications and backlight. Its printed pages 6-7
(PDF pages 7-8) cover calibration and communications settings.

The specified pH sequence is **4.00, then 6.86, then 9.18**. Clean/rinse the
electrode according to its instructions, immerse in the matching reference
buffer, wait for a stable reading, select the matching point and confirm.
Do not confuse storage solution with a calibration standard. Keep standards
separate to limit carryover, and use their temperature-dependent values.

The controller checks calibration parameters/electrode performance and may
show **Cal failed**. That message does not uniquely identify a damaged probe,
bad buffer, ADC fault or software-state problem. On this unit, the owner had
also reported the same symptom with a probe from a working controller.

Before reset, front-panel 6.86 calibration succeeded and measured 6.86-6.87;
returning to 9.18 still measured 5.78-5.81 and front-panel 9.18 calibration
failed. After reset, the response improved but a completed, independently
checked three-point calibration has not been recorded.

V1.5 lists address menu range 1-253 and baud choices 2400, 4800, 9600, 19200,
57600 and 115200. For interoperable Modbus use, keep normal addresses within
1-247. Only ID 1/2 and 9600/4800 transitions are established by this recovery.

## 5. RS485 / Modbus RTU basics

This controller uses **Modbus RTU over half-duplex RS485**, not Modbus TCP or
ASCII. The tested framing is **8 data bits, no parity, 1 stop bit**. Register
addresses below are zero-based protocol addresses, not 40001-style labels.
Use function **03** for holding-register reads and **06** for single-register
writes. A GUI asking for decimal address needs 48 for `0x0030`, 49 for
`0x0031`, and 55 for `0x0037`.

Multi-byte register addresses and values are big-endian. The Modbus RTU CRC-16
is transmitted low byte first. Change the slave ID or payload and the CRC must
be recomputed. The example frames below include CRC and are checked by the
repository test harness. See the
[Modbus application specification](https://www.modbus.org/file/secure/modbusprotocolspecification.pdf)
for function 03/06 and exception formats.

### Read examples

| Operation | Full RTU frame, hex |
| --- | --- |
| Read pH + EC, ID 1 | `01 03 00 00 00 02 C4 0B` |
| Read pH + EC + temperature, ID 1 | `01 03 00 00 00 03 05 CB` |
| Read pH + EC + temperature, ID 2 | `02 03 00 00 00 03 05 F8` |

V1.5's example payload, with its CRC corrected, is
`01 03 04 02 AE 01 94 9A 55`: two registers,
`0x02AE=686` gives pH 6.86, and the manual calls `0x0194=404` an EC of
404 uS/cm. The manual prints CRC `9A A5`; calculation requires **`9A 55`**.
That is a corrected manual example, not a live EC accuracy test.

Actual post-reset measurement reply:

```text
01 03 06 03 75 00 00 00 00 AC 8D
            ----- ----- -----
             pH    EC    temp
0x0375 = 885 -> pH 8.85; EC and temperature raw values are zero.
```

Validate slave, function, byte count, length and CRC, not CRC alone. A valid
function-06 echo only acknowledges the write; it does not prove a calibration
passed or settings survived a reboot.

### Response timing and discovery

Some observed replies took about **401 ms**. A fast sequential sweep with a
150 ms timeout misattributed late replies to the next register. Modbus read
replies do not carry the requested register address, so this can silently
produce a plausible but wrong register map.

The authoritative map uses paced single-register requests, bounded response
capture, and a quiet interval/drain. The updated sweep allows 500 ms plus a
250 ms inter-request gap. Recovery snapshots used longer raw capture windows.
These are empirical bench settings, not guaranteed worst-case device timing.

The V1.5/BGT manuals describe address **0xFE (254)** for vendor-specific
single-device discovery. This is **not standard Modbus broadcast address 0**.
It is not validated on this PCB. Do not use it on a multidrop bus or substitute
it into reset/write examples. The bench scanner only tries IDs 1-5 and bauds
9600/4800/19200, so "not found" is not an exhaustive bus search.

## 6. Register reference

Access reflects what is documented or observed, not a promise that all values
are safe. Do not write unknown calibration fields to match another controller.

| Address | Meaning / observation | Evidence and confidence |
| --- | --- | --- |
| 0x0000 | pH raw integer / 100 | Manual + live buffer/display comparison |
| 0x0001 | EC raw value; scale variant-dependent | Manuals conflict; live zero only |
| 0x0002 | Temperature / 10 C in related map | BGT manual; live zero only |
| 0x0030 | Slave ID | Manual; write 1 -> 2 and readback verified |
| 0x0031 | Baud encoding: 0x9600 or 0x4800 | Bench reset/readback and 0x4800 write verified |
| 0x0037 | Write 1: observed settings/calibration reset | Bench-tested once; full scope unknown |
| 0x0040-0x0047 | ASCII firmware identification | Readback: POET-251022-V4.2 |
| 0x0053 | EC correction/constant in newer BGT map | Related manual only; unverified here |
| 0x0054 | EC compensation in older BGT map | Related manual only; unverified here |
| 0x005E / 0x005F | EC electrode type/constant in older BGT map | Related manual only; unverified here |
| 0x0060 | Unknown mode; changing 1 -> 0 altered pH | Experimental; restored to 1 before reset |
| 0x0061 | Observed 250 during later tests | Meaning unverified |
| 0x0062-0x0064 | Calibration-related values | Changed with front-panel calibration and reset; exact meaning unknown |
| 0x0067 / 0x0068 | Observed 317 / 1565 around reset | Meaning unverified |
| 0x0069 | Older BGT calibration-point selector | Write echo/readback, but no effective calibration in our tested sequence |
| 0x006A | Outcome-like field in our captures; calibration trigger in newer BGT map | Conflicting semantics; do not equate zero with success |
| 0x006B-0x006D | Observed values changed with calibration/reset | May include live values; interpretation unverified |
| 0x006F | Older BGT channel selector (0=pH, 1=EC) | pH selection echoed; successful calibration not established |
| 0x007B | Newer BGT channel selector (1=pH, 2=EC) | Related manual; new sequence untested here |
| 0x00A0-0x00A4 | Undocumented readable block | Meanings unknown; A1=10000 is not proof of a password register |

The full captured low map is in
[the paced raw log](../evidence/2026-10-14_paced-raw-map.txt). Do not use the
earlier fast sweep as an authoritative map.

## 7. Changing address and baud without resetting

These are the exact verified steps used to restore this unit after reset:

1. At **ID 1 / 9600**, send `01 06 00 30 00 02 08 04`.
2. Reconnect at **ID 2 / 9600** and read `0x0030`; it must be 2.
3. At **ID 2 / 9600**, send `02 06 00 31 48 00 EE 36`.
4. Reconnect at **ID 2 / 4800** and read both `0x0030` and `0x0031`.
5. Confirm values 2 and **0x4800**, then read the measurement registers.

The baud value is **hex 0x4800 (decimal 18432)**, not decimal 4800 (0x12C0).
Do not invent encodings for the other menu rates. A serial-setting write can
make the old connection silent immediately; verify the new settings before
retrying. Calibration-related values were unchanged by these two writes.
Power-cycle retention has not yet been tested.

## 8. Reset and calibration compatibility

The [reset procedure](reset-recovery.md) uses `0x0037=1` and restores defaults.
It is an empirical finding for this particular hardware/firmware, absent from
the V1.5 manual, not an established universal factory-reset specification.

The BGT calibration maps differ by document revision:

| Reference | pH channel | pH point | Status on this controller |
| --- | --- | --- | --- |
| BGT 2023-08-18 | 0x006F=0 | 0x0069=1/2/3 | Echoed, but did not reproduce front-panel calibration |
| BGT New Ver., local filename dated 2025-11-04 | 0x007B=1 | 0x006A=1/2/3 | Full sequence not tested |
| V1.5 display manual | Front-panel menu | 4.00 / 6.86 / 9.18 | Neutral succeeded; alkaline failed before reset |

An isolated pre-reset write of `0x006A=1` was rejected, but the newer channel
selection was not performed first. That does not prove the full newer
sequence would fail. Do not advertise either serial sequence as a verified
calibration method for this unit. Details and manual errors are in the
[source comparison](../manuals/README.md).

## 9. What remains unknown

- Whether reset permanently repairs the underlying calibration issue.
- The exact meanings of reset sentinel 0xAAAA and calibration-state fields.
- Whether the reset erases other settings or affects outputs on other variants.
- Whether restored communications and calibration survive a power cycle.
- Correct EC scaling, EC accuracy and temperature compensation on this unit.
- Whether the newer BGT calibration sequence works on this firmware.
- Behavior on other HSY board revisions or POET firmware versions.

Keep the device out of unattended dosing/control service until verified in all
required reference buffers and after a power cycle. An improved alkaline
reading is useful evidence, not a substitute for measurement validation.
