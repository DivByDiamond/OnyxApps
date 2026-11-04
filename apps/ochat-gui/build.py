#!/usr/bin/env python3
"""Build ochat-gui with OnyxCC: app + lib/ochat + lib/oc2rpc + the otablet widget toolkit (ring 1)."""
import argparse
import subprocess
import sys
from pathlib import Path

APP = Path(__file__).resolve().parent
ROOT = APP.parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--ring1", action="store_true")
    args = parser.parse_args()
    tablet = ROOT / "apps/otablet"
    toolkit = [tablet / name for name in (
        "core/state.c", "core/render.c", "graphics/paint.c", "graphics/raster.c", "device/geometry.c")]
    sources = sorted(APP.rglob("*.c")) + toolkit + sorted((ROOT / "lib/ochat").rglob("*.c")) \
        + sorted((ROOT / "lib/oc2rpc").rglob("*.c"))
    command = [args.compiler] + (["--ring1"] if args.ring1 else []) + [
        "-I", str(APP), "-I", str(tablet), "-I", str(ROOT / "lib/ochat"), "-I", str(ROOT / "lib/oc2rpc"),
        "-o", args.output, *map(str, sources)]
    sys.exit(subprocess.run(command).returncode)


if __name__ == "__main__":
    main()
