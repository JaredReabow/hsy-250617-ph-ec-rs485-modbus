# Community discussions

Published on 2026-10-14 by u/JaredReabow. Each post is tailored to its community
and preserves the distinction between an observed reset effect and proven
calibration/repair. Repository ownership is disclosed.

The initial two posts were checked in public logged-out views after submission.
Their optional AI disclosure sentences were subsequently removed at the owner's
request, with the required r/esp32 AI Content flair retained. Updated text was
verified in the browser. New reverse-engineering and hardware-hacking posts
explicitly explain AI assistance; both new permalinks were also verified in
logged-out views with no removal notice visible. Future moderation or indexing
is not guaranteed. This page records the latest text; Git history retains the
original versions.

## r/esp32

[ESP32-S3 RS485 bench tool: HSY-250617 pH controller reset and a delayed-reply Modbus trap](https://www.reddit.com/r/esp32/comments/1x5jsbc/esp32s3_rs485_bench_tool_hsy250617_ph_controller/)

Flair: AI Content. The community rules acknowledgment was submitted before posting.

### Published text

I used a Waveshare ESP32-S3-Relay-1CH as a USB-to-RS485 diagnostic tool for an unbranded 5 in 1 water quality sensor / pH/EC controller marked HSY-250617-v2.3 (firmware string POET-251022-V4.2). I'm publishing the source and captures because the available manuals don't agree on calibration registers.

The ESP32 setup uses UART1 TX17/RX18/DE21 with hardware half-duplex. The bench firmware has bounded raw-PDU capture, timestamps and CRC checks, with no automatic write retries. Discovery and normal polling are read-only.

One useful protocol lesson: some responses took about 401 ms. My earlier 150 ms register sweep could mistake a late reply for the next register's response. Modbus read replies don't include the requested register address, so the bad map looked plausible. Paced requests exposed the problem; the repository explicitly flags the old sweep as unreliable.

On this one controller, function 06 writing 1 to register 0x0037 produced a settings/calibration reset-like effect. It returned to ID 1 / 9600 / 8N1 and changed calibration-related registers. With the probe still in 9.18 buffer, pH moved from about 5.78 to 8.85. I subsequently restored ID 2 / 4800 using 0x0030=2 and 0x0031=0x4800, verifying each transition by readback.

Important: this is not a harmless reboot or a universal repair. Isolate dosing/control loads, save settings, and recalibrate. Full accuracy, power-cycle persistence and long-term repair remain unverified. A write echo is not proof of calibration success.

Source, product/manual images, wiring diagrams, reference guide and raw evidence: https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus

Results above are from the connected hardware; the included v1.0.4 source is build/native-tested, while the live recovery used v1.0.3. This is my repository, not a sales or affiliate link.

## r/diyelectronics

[HSY-250617-v2.3 pH/EC controller: documented RS485 reset, register map and conflicting calibration manuals](https://www.reddit.com/r/diyelectronics/comments/1x5jnog/hsy250617v23_phec_controller_documented_rs485/)

Flair: Reference.

### Published text

I have been troubleshooting an unbranded 5 in 1 water quality sensor / pH/EC controller with HSY-250617-v2.3 printed on the PCB and POET-251022-V4.2 reported as its firmware. It would accept a 6.86 calibration but failed at 9.18, reading roughly 5.8 with the electrode in 9.18 buffer.

I used a Waveshare ESP32-S3 RS485 board to capture transactions and compiled a public reference for anyone who finds this board marking later:
https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus

Findings from this unit:

- Modbus function 06, register 0x0037, value 1 caused a settings/calibration reset-like effect. Communications reverted to ID 1, 9600 baud, 8N1. This is not just a reboot.
- With the probe left in the same 9.18 buffer, the reading changed from about 5.78 to 8.85. That suggests stored calibration/settings were involved, but does not prove the hardware is healthy or fully calibrated.
- I restored ID 2 / 4800 baud through 0x0030=2 followed by 0x0031=0x4800, checking communications after each change. Later reads were 9.29-9.30; the guide preserves the chronology rather than treating that as proof of accurate calibration.
- The related manuals disagree on calibration registers and sequences. Their relationship to this exact PCB is unproven. One V1.5 example even has a bad response CRC: its printed 9A A5 should be 9A 55.
- Some replies arrived around 401 ms late, making an early fast register scan misleading. The paced captures are the useful evidence.

The repo contains the detailed register/reference guide, reset and recovery frames, original wiring diagrams, manual source links and revision comparison, bench firmware and raw captures. Product images supplied by me and extracted from the manual are now included, along with the four-wire map: red +, black -, yellow RS485 A, green RS485 B. Full third-party PDFs are linked rather than mirrored; there are no bench-PCB photographs yet.

Do not try resets on a live dosing system: isolate pumps/relays and save settings first. A write echo is not calibration success; full buffer validation, power-cycle persistence and long-term repair are still unverified. No universal fix is claimed.

This is my noncommercial repository. The reported responses came from real connected hardware. Has anyone encountered this exact PCB/firmware combination or a matching manufacturer manual?

## r/ReverseEngineering

[Black-box Modbus RE of a 5-in-1 water-quality controller: AI-assisted reset discovery and a 401 ms reply trap](https://www.reddit.com/r/ReverseEngineering/comments/1x5lkwz/blackbox_modbus_re_of_a_5in1_waterquality/)

Published as a link post directly to the original
[technical case study](https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus/blob/main/docs/reverse-engineering.md),
in accordance with the community's source-link and no-question-post rules.
No separate body or first comment. The linked article explains the AI/human
roles, bounded probing, failed experiments, exact frames and limits.

## r/hardwarehacking

[Recovering a no-brand 5 in 1 water quality sensor through RS485: AI-assisted probing found a settings reset](https://www.reddit.com/r/hardwarehacking/comments/1x5llqh/recovering_a_nobrand_5_in_1_water_quality_sensor/)

Published as an original text post after checking the community's relevance,
tech-support, English-language, ethical-use and no-phishing rules.

### Published text

I had a cheap unbranded pH/EC controller that accepted a 6.86 calibration but read about 5.8 in 9.18 buffer and failed alkaline calibration. PCB marking: HSY-250617-v2.3. Firmware string: POET-251022-V4.2. Rather than assuming the board was dead, I investigated its poorly documented Modbus interface.

I connected it to a Waveshare ESP32-S3-Relay-1CH over RS485 and used an AI coding assistant to build the bench tool, compare manuals, propose candidate commands and iterate against real replies. I did the wiring, moved the electrode and checked the physical display. I asked it to brute-force a recovery, but the actual work was bounded candidate probing, not every register/value combination and not a firmware dump or authentication bypass.

The useful hit was function 06, register 0x0037, value 1. At ID 2 / 4800 the full frame and echo were: 02 06 00 37 00 01 F9 F7. The controller stopped answering at the old settings and was later rediscovered at ID 1 / 9600 / 8N1. Calibration-related registers changed from 569 / 659 / 733 to 0xAAAA / 0 / 0, and with the probe left in the same 9.18 buffer the display and serial reading moved to about 8.85.

The biggest trap was in our tooling: replies sometimes arrived around 401 ms late. A 150 ms sweep assigned genuine, CRC-valid responses to the next requested register. Function-03 replies do not echo the register address, so the false map looked believable. Paced transactions exposed it. Several calibration/control writes also echoed without solving anything; one attempted restoration was rejected. I have kept those failures in the write-up.

I then restored ID 2 / 4800 using 0x0030=2 followed by 0x0031=0x4800, checking each transition. Later readings of 9.29-9.30 are recorded separately, not claimed as the immediate reset result or proof of calibration.

Wiring on this unit: red supply +, black supply -, yellow RS485 A, green RS485 B. The ESP32 board uses UART1 TX17/RX18/DE21 with hardware half-duplex. Verify supply voltage and use an RS485 transceiver, not direct TTL UART wiring.

This is a settings/calibration reset, not a harmless reboot or universal fix. Isolate dosing/control outputs, save settings and recalibrate; no shared-bus broadcast sweeps. Full accuracy, power-cycle persistence and long-term repair remain unverified.

My noncommercial repository includes the case study, raw captures, product/manual images, manual revision comparison and bench source:
https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus/blob/main/docs/reverse-engineering.md

The live recovery used bench firmware v1.0.3; the published v1.0.4 source is build/native-tested, not newly flashed. If you have the same PCB/firmware, comparisons of paced register reads and exact manual revisions would be useful.
