"""Offline publication checks plus the existing native Modbus protocol harness."""

from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def publication_crc(data):
    """Return CRC-16/Modbus for documentation example verification."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc


class PublicationTests(unittest.TestCase):
    """Validate reference integrity without interacting with any hardware."""

    def test_native_protocol(self):
        """Compile and run the protocol tests in an isolated temporary folder."""
        with tempfile.TemporaryDirectory() as folder:
            binary = str(Path(folder) / "protocol-tests")
            subprocess.run([
                "c++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                "-I", str(ROOT / "firmware/include"),
                str(ROOT / "firmware/test/native_protocol.cpp"), "-o", binary,
            ], check=True)
            subprocess.run([binary], check=True)

    def test_documented_frames(self):
        """Every full hexadecimal RTU example in guide prose must have valid CRC."""
        count = 0
        for path in [ROOT / "docs/controller-guide.md", ROOT / "docs/reset-recovery.md"]:
            for match in re.finditer(r"(?<![A-Za-z0-9])(?:[0-9A-F]{2} ){4,}[0-9A-F]{2}(?![A-Za-z0-9])", path.read_text()):
                frame = bytes.fromhex(match.group())
                self.assertEqual(publication_crc(frame), 0, f"{path.name}: {match.group()}")
                count += 1
        self.assertGreaterEqual(count, 8)

    def test_relative_links(self):
        """Check Markdown file/image targets without requesting external sites."""
        for path in ROOT.rglob("*.md"):
            if ".pio" in path.parts:
                continue
            for target in re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", path.read_text()):
                if target.startswith(("http://", "https://", "#", "mailto:")):
                    continue
                self.assertTrue((path.parent / target.split("#")[0]).exists(), f"{path}: {target}")

    def test_manual_crc_erratum(self):
        """Preserve the manual's invalid CRC as an explicit negative test."""
        self.assertNotEqual(publication_crc(bytes.fromhex("01 03 04 02 AE 01 94 9A A5")), 0)
        self.assertEqual(publication_crc(bytes.fromhex("01 03 04 02 AE 01 94 9A 55")), 0)

    def test_diagrams(self):
        """Require well-formed, self-contained SVG assets with accessible titles."""
        for path in (ROOT / "assets").glob("*.svg"):
            root = ET.parse(path).getroot()
            self.assertIsNotNone(root.find("{http://www.w3.org/2000/svg}title"))
            self.assertNotIn("<script", path.read_text())

    def test_recovery_evidence(self):
        """Require exact reset/restore frames and resulting readings in raw logs."""
        reset = (ROOT / "evidence/2026-10-14_control-0037.txt").read_text()
        self.assertIn("02 06 00 37 00 01 F9 F7", reset)
        restore = (ROOT / "evidence/2026-10-14_restore-comms.txt").read_text()
        for marker in ["01 06 00 30 00 02 08 04", "02 06 00 31 48 00 EE 36",
                       "addr 2 @ 4800 baud", "02 03 02 48 00", "pH=9.29", "pH=9.30"]:
            self.assertIn(marker, restore)
        post = (ROOT / "evidence/2026-10-14_post-reset-registers.txt").read_text()
        self.assertIn("01 03 02 AA AA", post)

    def test_public_text_screen(self):
        """Reject common credential patterns and accidental private workspace paths."""
        patterns = [r"/Users/[^/\s]+/", r"gh[pousr]_" + r"[A-Za-z0-9]{20,}",
                    r"-----BEGIN " + r"(?:RSA |OPENSSH )?PRIVATE KEY-----",
                    r"(?i)(?:wifi_password|api_key)\s*=\s*[\"'][^\"']+[\"']"]
        for path in ROOT.rglob("*"):
            if not path.is_file() or any(p in path.parts for p in [".git", ".pio", "__pycache__"]):
                continue
            if path.suffix not in [".md", ".txt", ".cpp", ".h", ".ini", ".svg"]:
                continue
            content = path.read_text()
            for pattern in patterns:
                self.assertIsNone(re.search(pattern, content), f"Private-data pattern in {path}")


if __name__ == "__main__":
    unittest.main(verbosity=2)
