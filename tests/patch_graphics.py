#!/usr/bin/env python3
"""Test exact external Graphics binary patches without Apple binaries."""
import importlib.util
from pathlib import Path
import tempfile


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("patch_graphics", ROOT / "tools" / "patch_graphics.py")
patch_graphics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch_graphics)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="wex-graphics-patch-") as directory:
        root = Path(directory)
        mtl = root / "mtl"
        mtl.write_bytes(b"\0" * 0x7ED87 + b"\x75\x0D" + b"\0" * 8)
        output = root / "mtl-out"
        patch_graphics.patch_file(mtl, output, "mtl-leak")
        assert output.read_bytes()[0x7ED87:0x7ED89] == b"\x90\x90"

        va = root / "va"
        data = bytearray(b"\0" * 0x24361F + b"ICL" + b"\0" * (0x24364F - 0x24361F - 3) + b"ICL")
        va.write_bytes(data)
        va_output = root / "va-out"
        patch_graphics.patch_file(va, va_output, "tgl-vame-lookup")
        patched = va_output.read_bytes()
        assert patched[0x24361F:0x243622] == b"TGL"
        assert patched[0x24364F:0x243652] == b"TGL"

        try:
            patch_graphics.patch_file(va, va, "tgl-vame-lookup")
        except ValueError as error:
            assert "in place" in str(error)
        else:
            raise AssertionError("in-place patch was accepted")

        bad = root / "bad"
        bad.write_bytes(b"\0" * 0x7ED87 + b"\x90\x90")
        try:
            patch_graphics.patch_file(bad, root / "bad-out", "mtl-leak")
        except ValueError as error:
            assert "expected 750d" in str(error)
        else:
            raise AssertionError("unexpected binary was accepted")
    print("PASS: exact external Graphics patches, output isolation and guards")


if __name__ == "__main__":
    main()
