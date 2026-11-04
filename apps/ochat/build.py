#!/usr/bin/env python3
"""Build ochat with OnyxCC: app sources + lib/ochat + lib/oc2rpc (RPC bus)."""
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
    sources = sorted(APP.rglob("*.c")) + sorted((ROOT / "lib/ochat").rglob("*.c")) \
        + sorted((ROOT / "lib/oc2rpc").rglob("*.c"))
    command = [args.compiler] + (["--ring1"] if args.ring1 else []) + [
        "-I", str(APP), "-I", str(ROOT / "lib/ochat"), "-I", str(ROOT / "lib/oc2rpc"),
        "-o", args.output, *map(str, sources)]
    sys.exit(subprocess.run(command).returncode)


if __name__ == "__main__":
    main()
