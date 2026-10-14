# History (append-only)

## 2026-10-14: initial public documentation preparation

Created a separate public-reference checkout from selected bench source and
evidence, not a publication of the private workspace or its Git history.
Read the local V1.5 and newer BGT PDFs; compared their maps with live captures.
Documented the reset effect, current restored settings and remaining accuracy/
persistence limits. Original diagrams label conceptual wiring and test stages.
Manual redistribution permission and original controller photo files requested.
No controller commands or firmware flashes were performed for publication.

## 2026-10-14: publication validation

Seven offline harness checks passed, including native C++ protocol tests,
documentation CRCs, the V1.5 CRC erratum, local links, SVG structure, primary
evidence markers and a private-data pattern screen. Both original diagrams
were rendered and visually inspected; a clipped supply caption was corrected.
Fresh PlatformIO build passed with platform 55.3.37 / Arduino 3.3.7, RAM 22232
bytes and flash program usage 354724 bytes. Build only; no new flash.
Git commits use the account's GitHub no-reply address for this public checkout.

## 2026-10-14: public repository and community publication

Published the separate reference repository as JaredReabow/hsy-250617-ph-ec-rs485-modbus,
with search-oriented description and topics. Submitted one original technical post
to r/esp32 (AI Content flair) and one to r/diyelectronics (Reference flair).
Completed the r/esp32 rules acknowledgment before submission. Both post bodies,
authors, flairs and GitHub links were verified through public logged-out views;
neither showed a removal notice at the time checked. Retained exact post text
and permalinks in docs/community-posts.md. No further hardware writes or flashes.

## 2026-10-14: product naming, wiring and manual images

At the owner's request, added the exact search wording "5 in 1 water quality
sensor" prominently, alongside the existing PCB identifier. Highlighted the
owner-confirmed four-wire map: red positive, black negative, yellow RS485 A,
green RS485 B. Extracted three unchanged embedded JPEGs from V1.5 PDF pages
1, 3 and 5 for product recognition, dimensions and wiring. Source attribution
and third-party license exclusions are explicit; no bench-PCB photo or verified
five-channel capability is implied. No firmware or controller-state changes.

The owner then supplied a 750 x 750 product image. Included it unchanged as
the README lead picture, with provenance and visible-variant caveats; retained
the manual product images in the controller guide.

## 2026-10-14: reverse-engineering case study

Added a technical article explaining AI-assisted candidate probing, physical
owner observations, the 401 ms late-response attribution trap, failed experiments,
the observed reset and separately verified communications restoration. Explicitly
does not claim exhaustive brute force, firmware extraction, multiple reset modes,
or complete calibration/repair. No new controller commands or firmware flashes.
