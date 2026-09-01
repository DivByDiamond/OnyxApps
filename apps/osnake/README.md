# OnyxApps / osnake

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Classic snake game for the OnyxOS terminal, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS).

----

## Features

- Full-screen ANSI board (32x16 grid), ASCII glyphs only (font-safe)
- Arrows or WASD steering, space pauses, q quits
- Walls and self-contact are lethal; food grows the snake (+10 points)
- Auto speed-up as the score grows; explicit tick interval via argv
- Deterministic LCG food sequence (seeded from clock + PID)
- Game over / win overlays (fill the whole board to win)

## Build

```sh
make osnake                # from the repo root (onyxcc in PATH or ONYXCC=...)
onyxcc -I apps/osnake -o osnake.onx apps/osnake/main.c apps/osnake/game/*.c apps/osnake/ui/*.c
```

## Usage

```sh
osnake            # auto speed (160 ms tick, ramping down to 70 ms)
osnake 200        # fixed 200 ms tick
```

| Key | Action |
|-----|--------|
| arrows / WASD | steer |
| space / p | pause |
| q / Ctrl+C | quit |

## Requirements

Works on the kernel's ANSI/VT100 terminal (fb_term/ansi): colors SGR
30-37/90-97, cursor positioning `CSI H`, erase `J/K`, per-process termios
via TCGETS/TCSETS (raw mode, `cfmakeraw_apply`, VMIN=0/VTIME=0 for
nonblocking reads), TIOCGWINSZ returning the real framebuffer grid -
provided by OnyxKernel v0.4+.

## Testing

The `game/` modules are pure C99 with no I/O and are covered by host
tests that run on any Linux with gcc (no OnyxOS required):

```sh
make -C tests/native test      # runs test_osnake and test_otop
```

Covered: reset invariants, wall death, self death, 180-degree reversal
rejection, growth and scoring, full-board food spawn failure (win path),
RNG determinism, 3000-step fuzz run with invariants.

## Project structure

```text
apps/osnake/
├── README.md     this document
├── osnake.h      shared state (struct snake) + module API
├── main.c        program entry: args, key dispatch, frame loop
├── game/         logic.c  food.c
│                 (pure state transitions, LCG RNG + food placement)
└── ui/           render.c  input.c
                  (ANSI drawing, raw term, nonblocking key decode)
```

Licensed under GPL-3.0-or-later.
