# OnyxApps / osysmon

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

System monitor (btop/htop-style), part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS).

----

## Features

- Full-screen box-drawing UI with colored load bars
- System header: uname, uptime
- Process panel: PID, state, CWD
- Disk and network panels
- Periodic refresh by interval

## Build

```sh
make osysmon                # from the repo root (onyxcc in PATH or ONYXCC=...)
onyxcc -o osysmon.onx apps/osysmon/osysmon.c   # manual; libonyxc is auto-linked
```

## Usage

```sh
osysmon
```

Runs until interrupted (`Ctrl+C`); refreshes the panels by interval.

## Requirements

Works on the kernel's ANSI/VT100 terminal (fb_term/ansi): colors SGR
30-37/90-97, cursor positioning `CSI H`, erase `J/K`, box-drawing glyphs,
per-process termios via TCGETS/TCSETS, TIOCGWINSZ returning the real
framebuffer grid - provided by OnyxKernel v0.4+.

## Notes

- Migrated as a single file from OnyxOS `software/` (v0.6); splitting it
  to the 200-line project layout is pending

Licensed under GPL-3.0-or-later.
