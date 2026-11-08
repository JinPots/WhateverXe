#!/usr/bin/env python3
"""Apply exact, opt-in fixes to user-supplied WhateverXe Graphics binaries."""
import argparse
import os
from pathlib import Path
import tempfile


PATCHES = {
    "mtl-leak": ((0x7ED87, b"\x75\x0D", b"\x90\x90"),),
    "tgl-vame-lookup": (
        (0x24361F, b"ICL", b"TGL"),
        (0x24364F, b"ICL", b"TGL"),
    ),
}


def patch_file(source: Path, output: Path, patch_set: str) -> None:
    source = source.resolve()
    output = output.resolve()
    if source == output:
        raise ValueError("refusing to patch a binary in place; choose a new output path")
    data = source.read_bytes()
    changes = PATCHES[patch_set]
    for offset, before, _ in changes:
        actual = data[offset:offset + len(before)]
        if actual != before:
            raise ValueError(
                f"{patch_set}: {source} has {actual.hex()} at 0x{offset:X}; "
                f"expected {before.hex()}"
            )
    patched = bytearray(data)
    for offset, _, after in changes:
        patched[offset:offset + len(after)] = after
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as handle:
        temporary = Path(handle.name)
        handle.write(patched)
    try:
        os.chmod(temporary, source.stat().st_mode)
        temporary.replace(output)
    finally:
        if temporary.exists():
            temporary.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mtl-driver", type=Path, help="input AppleIntelTGLGraphicsMTLDriver")
    parser.add_argument("--mtl-output", type=Path, help="patched Metal driver output")
    parser.add_argument("--va-driver", type=Path, help="input AppleIntelTGLGraphicsVADriver")
    parser.add_argument("--va-output", type=Path, help="patched VA driver output")
    args = parser.parse_args()
    if bool(args.mtl_driver) != bool(args.mtl_output):
        parser.error("--mtl-driver and --mtl-output must be provided together")
    if bool(args.va_driver) != bool(args.va_output):
        parser.error("--va-driver and --va-output must be provided together")
    if not args.mtl_driver and not args.va_driver:
        parser.error("provide at least one driver and its output path")
    try:
        if args.mtl_driver:
            patch_file(args.mtl_driver, args.mtl_output, "mtl-leak")
            print(f"patched mtl-leak: {args.mtl_output}")
        if args.va_driver:
            patch_file(args.va_driver, args.va_output, "tgl-vame-lookup")
            print(f"patched tgl-vame-lookup: {args.va_output}")
    except (OSError, ValueError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
