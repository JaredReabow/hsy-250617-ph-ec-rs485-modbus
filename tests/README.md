# Local test harness

Run from the repository root with Python 3 and a C++11 compiler installed:

```sh
python3 tests/validate.py
```

This runs the native protocol harness, validates documented full-frame CRCs,
checks local Markdown links and SVG XML, confirms key raw-evidence claims, and
screens publishable text for common secrets/private paths. It does not prove
hardware safety, exhaustive privacy or device accuracy. No serial connection,
network request, controller write or firmware upload occurs.

For a firmware build, separately run `pio run -d firmware`. No paid CI or
automatic hardware writes are configured.
