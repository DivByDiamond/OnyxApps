#!/usr/bin/env python3
"""Build the standard Lua interpreter with OnyxCC."""
import sys
sys.dont_write_bytecode = True
from port.build import build

if __name__ == "__main__":
    build("lua")
