# OnyxApps / oed

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Full-screen text editor (nano-style), part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS).

----

## Features

- Full-screen ANSI interface: line numbers, `~` markers for empty lines,
  status bar (mode, dirty indicator, line/column counters)
- Movement: arrows, Home/End, PageUp/PageDown
- Editing: printable characters, Enter, Backspace, Delete, Tab (4 spaces)
- Scrolling with a viewport window over the buffer
- `Ctrl+S` save, `Ctrl+Q` quit (double press on unsaved changes),
  `Ctrl+G` help screen
- Buffer: up to 2048 lines x 512 bytes

## Build

```sh
make oed                    # from the repo root (onyxcc in PATH or ONYXCC=...)
onyxcc -I apps/oed -o oed.onx apps/oed/src/*.c   # manual; libonyxc is auto-linked
```

## Usage

```sh
oed /etc/passwd        # open a file
oed                    # start with an empty buffer
```

| Key | Action |
|-----|--------|
| arrows / Home / End / PgUp / PgDn | movement |
| printable chars, Enter, Backspace, Delete, Tab | editing |
| `Ctrl+S` | save |
| `Ctrl+Q` | quit (double press when dirty) |
| `Ctrl+G` | help screen |

## Requirements

Works on the kernel's ANSI/VT100 terminal (fb_term/ansi): colors SGR
30-37/90-97, cursor positioning `CSI H`, erase `J/K`, per-process termios
via TCGETS/TCSETS (raw mode, `cfmakeraw`), TIOCGWINSZ returning the real
framebuffer grid - provided by OnyxKernel v0.4+.

## Notes

- Migrated as a single file from OnyxOS `software/` (v0.6); now split into
  modules following the 200-line project layout (see Project structure)

Licensed under GPL-3.0-or-later.

## Project structure

```text
apps/oed/
  README.md    this file
  oed.h        shared state (extern) and module API
  src/
    main.c     key decoding (read_key) and the editor main loop
    edit.c     buffer state, editing primitives, file I/O
    screen.c   terminal control (raw mode, size) and rendering
```
