#!/usr/bin/env python3
"""Build the standard Lua bytecode compiler using the shared pinned source."""
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lua"))
from port.build import build

if __name__ == "__main__":
    build("luac")
