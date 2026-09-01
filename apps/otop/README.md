# OnyxApps / otop

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

btop-style system monitor for OnyxOS, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS).

Unlike osysmon (which renders some synthetic demo bars), every number in
otop comes from the kernel: the `/proc` pseudo files (meminfo, load,
cpuinfo, uptime) filled by OnyxKernel `fs/procfs`, `uname`, and the
process's own heap via `sbrk(0)`.

----

## Features

- Memory panel: RAM used/total, kernel heap used/total, own heap - all
  with colored load bars (green < 50%, yellow < 80%, red above)
- Load panel: process count, running/blocked, harts (cpus), uptime
- Heap history panel: ASCII sparkline of the last 48 samples with
  min/max/current stats
- Header with host name, kernel release and a refresh spinner
- Keys: `q` quit, `space` pause (freezes frame + history), `+`/`-`
  change the refresh interval (100 ms .. 5000 ms)
- Missing `/proc` data renders as `(n/a)` instead of crashing

## Build

```sh
make otop                  # from the repo root (onyxcc in PATH or ONYXCC=...)
onyxcc -I apps/otop -o otop.onx apps/otop/main.c apps/otop/probe/*.c apps/otop/ui/*.c
```

## Usage

```sh
otop             # refresh every 1000 ms
otop 250         # refresh every 250 ms
```

| Key | Action |
|-----|--------|
| q / Ctrl+C | quit |
| space | pause / resume |
| + / - | slower / faster refresh |

## Requirements

Works on the kernel's ANSI/VT100 terminal (fb_term/ansi): colors SGR
30-37/90-97, cursor positioning `CSI H`, erase `J/K`, per-process
termios via TCGETS/TCSETS (raw mode, `cfmakeraw_apply`), TIOCGWINSZ
returning the real framebuffer grid, and the `/proc` files
(`/proc/meminfo`, `/proc/load`, `/proc/cpuinfo`, `/proc/uptime`) -
provided by OnyxKernel v0.4+.

## Testing

The parser/format module (`probe/parse.c`) is pure C99 with no I/O and
is covered by host tests that run on any Linux with gcc (no OnyxOS
required):

```sh
make -C tests/native test      # runs test_osnake and test_otop
```

Covered: meminfo/load/cpuinfo field parsing (exact kernel formats),
uptime parsing, permille edge cases (zero total, overflow clamp),
uptime formatting, history ring behavior, sparkline output.

## Project structure

```text
apps/otop/
├── README.md     this document
├── otop.h        shared types (snapshot, history) + module API
├── main.c        program entry: terminal control, keys, refresh loop
├── probe/        data.c  parse.c
│                 (syscall probes incl. /proc readers, pure parsers)
└── ui/           draw.c  panels.c
                  (boxes/bars/header/footer, panel layout)
```

Licensed under GPL-3.0-or-later.
