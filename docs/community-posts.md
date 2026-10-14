# Community discussions

Published on 2026-10-14 by u/JaredReabow. Each post is tailored to its community,
discloses AI assistance and repository ownership, and preserves the distinction
between an observed reset effect and proven calibration/repair.

Both permalinks were checked through a public, logged-out web view after
submission: title, author, flair, body and GitHub link were present. No removal
notice was visible at that check. Future moderation or indexing is not guaranteed.

## r/esp32

[ESP32-S3 RS485 bench tool: HSY-250617 pH controller reset and a delayed-reply Modbus trap](https://www.reddit.com/r/esp32/comments/1x5jsbc/esp32s3_rs485_bench_tool_hsy250617_ph_controller/)

Flair: AI Content. The community rules acknowledgment was submitted before posting.

### Published text

I used a Waveshare ESP32-S3-Relay-1CH as a USB-to-RS485 diagnostic tool for an unbranded pH/EC controller marked HSY-250617-v2.3 (firmware string POET-251022-V4.2). I'm publishing the source and captures because the available manuals don't agree on calibration registers.

The ESP32 setup uses UART1 TX17/RX18/DE21 with hardware half-duplex. The bench firmware has bounded raw-PDU capture, timestamps and CRC checks, with no automatic write retries. Discovery and normal polling are read-only.

One useful protocol lesson: some responses took about 401 ms. My earlier 150 ms register sweep could mistake a late reply for the next register's response. Modbus read replies don't include the requested register address, so the bad map looked plausible. Paced requests exposed the problem; the repository explicitly flags the old sweep as unreliable.

On this one controller, function 06 writing 1 to register 0x0037 produced a settings/calibration reset-like effect. It returned to ID 1 / 9600 / 8N1 and changed calibration-related registers. With the probe still in 9.18 buffer, pH moved from about 5.78 to 8.85. I subsequently restored ID 2 / 4800 using 0x0030=2 and 0x0031=0x4800, verifying each transition by readback.

Important: this is not a harmless reboot or a universal repair. Isolate dosing/control loads, save settings, and recalibrate. Full accuracy, power-cycle persistence and long-term repair remain unverified. A write echo is not proof of calibration success.

Source, wiring diagrams, reference guide and raw evidence: https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus

Disclosure: the bench tooling, investigation and write-up were AI-assisted. Results above are from the connected hardware; the included v1.0.4 source is build/native-tested, while the live recovery used v1.0.3. This is my repository, not a sales or affiliate link.

## r/diyelectronics

[HSY-250617-v2.3 pH/EC controller: documented RS485 reset, register map and conflicting calibration manuals](https://www.reddit.com/r/diyelectronics/comments/1x5jnog/hsy250617v23_phec_controller_documented_rs485/)

Flair: Reference.

### Published text

I have been troubleshooting an unbranded pH/EC controller with HSY-250617-v2.3 printed on the PCB and POET-251022-V4.2 reported as its firmware. It would accept a 6.86 calibration but failed at 9.18, reading roughly 5.8 with the electrode in 9.18 buffer.

I used a Waveshare ESP32-S3 RS485 board to capture transactions and compiled a public reference for anyone who finds this board marking later:
https://github.com/JaredReabow/hsy-250617-ph-ec-rs485-modbus

Findings from this unit:

- Modbus function 06, register 0x0037, value 1 caused a settings/calibration reset-like effect. Communications reverted to ID 1, 9600 baud, 8N1. This is not just a reboot.
- With the probe left in the same 9.18 buffer, the reading changed from about 5.78 to 8.85. That suggests stored calibration/settings were involved, but does not prove the hardware is healthy or fully calibrated.
- I restored ID 2 / 4800 baud through 0x0030=2 followed by 0x0031=0x4800, checking communications after each change. Later reads were 9.29-9.30; the guide preserves the chronology rather than treating that as proof of accurate calibration.
- The related manuals disagree on calibration registers and sequences. Their relationship to this exact PCB is unproven. One V1.5 example even has a bad response CRC: its printed 9A A5 should be 9A 55.
- Some replies arrived around 401 ms late, making an early fast register scan misleading. The paced captures are the useful evidence.

The repo contains the detailed register/reference guide, reset and recovery frames, original wiring diagrams, manual source links and revision comparison, bench firmware and raw captures. Full third-party PDFs and actual hardware photographs are not included yet.

Do not try resets on a live dosing system: isolate pumps/relays and save settings first. A write echo is not calibration success; full buffer validation, power-cycle persistence and long-term repair are still unverified. No universal fix is claimed.

This is my noncommercial repository. The investigation, bench tooling and documentation were AI-assisted; the reported responses came from real connected hardware. Has anyone encountered this exact PCB/firmware combination or a matching manufacturer manual?

