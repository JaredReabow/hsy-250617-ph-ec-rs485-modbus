# Bench evidence, 2026-10-14

Raw serial-console captures from the isolated controller investigation. They
are historical observations, not command scripts to execute. No private chat,
USB device identity, Wi-Fi credentials or unrelated project history is needed
to reproduce the documented findings.

## Primary recovery chain

| Capture | Use |
| --- | --- |
| [paced-raw-map](2026-10-14_paced-raw-map.txt) | Authoritative paced pre-experiment register identity |
| [control-0037](2026-10-14_control-0037.txt) | Reset request and valid echo |
| [after-0037-rescan](2026-10-14_after-0037-rescan.txt) | Initial unsuccessful rediscovery |
| [reset-defaults-reconnect](2026-10-14_reset-defaults-reconnect.txt) | Found ID 1 / 9600, pH 8.85 |
| [post-reset-registers](2026-10-14_post-reset-registers.txt) | Fresh paced read-only reset snapshot |
| [restore-comms](2026-10-14_restore-comms.txt) | Two setting writes, ID 2 / 4800 readback and three pH reads |

## Calibration and diagnostic records

The `front-panel-neutral-*` captures accompany the owner's successful neutral
calibration report. `alkaline-after-neutral*` and `front-panel-alkaline-failed`
record the continuing pre-reset alkaline failure. `recovery-*` contains serial
point-selection attempts, not successful calibration claims.

The standard-diagnostics, FC16, A2/A3, mode, result and password-candidate files
record bounded experiments. Most did not repair anything; some were rejected.
See [the chronology](../docs/reset-recovery.md) rather than inferring a recipe
from filenames or a write echo. A0/A1 meanings remain unknown.

**Do not trust `2026-10-14_pre-experiment-map.txt` as a register map.** Its fast
sweep could assign a delayed response to the next request. It is preserved
for transparency and superseded by the paced raw map. Other early fast sweeps
are discovery hints only unless individually confirmed.

The reported timestamps are bench-local capture timestamps. The publication
date is 2026-10-14, Australia/Sydney. No power-cycle persistence result is present.
