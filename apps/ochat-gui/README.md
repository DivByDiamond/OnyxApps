# OnyxApps / ochat-gui

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Graphical chat for the OC2R radio network, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS). Runs on
`/dev/fb0` with the mouse, using the otablet widget toolkit on a 240x112
logical screen scaled to the framebuffer.

## Build

```sh
make ochat-gui ONYXCC=../OnyxCompiller/onyxcc
# or, manually:
python3 apps/ochat-gui/build.py --compiler ../OnyxCompiller/onyxcc --output build/ochat-gui.onx
```

Runs in the user ring. Pulls in `lib/ochat`, `lib/oc2rpc` and the otablet
toolkit (`core/state.c`, `core/render.c`, `graphics/paint.c`,
`graphics/raster.c`, `device/geometry.c`).

## Usage

```sh
ochat-gui            # no arguments; needs /dev/fb0, the mouse and the RPC bus
```

Left: device list (click to select; `!` unread, `*` online, `-` offline,
`>` selected), UP/DOWN scroll, PING and REFR (refresh list). Right: history
with the selected device (green = sent). Bottom: input line typed on the
terminal keyboard; Enter or SEND sends. Top right: ping indicator
(`PING 12.3MS 0%` = last RTT and loss, dot colour = state).

Tab cycles contacts; `/to ADDRESS` opens a chat with an address not in the
list; `/ping` pings the selected contact. Right click or Ctrl-C exits. The
GUI shows ASCII only (other characters appear as `?`). Keyboard input is read
from stdin, so start it from the console that has the keyboard focus.

## Requirements

- `/dev/fb0` framebuffer, the mouse (absolute coordinates via syscall 86)
  and the RPC bus `/dev/hvc0`. One program may own the bus at a time (see
  `ochat`).
- Runs on the kernel's framebuffer/mouse ABI; unsupported or missing devices
  return failure.

## Testing

The GUI model is covered by host tests with a fake radio backend:

```sh
make -C tests/native test      # includes test_ochat_gui
```

The framebuffer loop and transport need a live OnyxOS guest.

## Project structure

```text
apps/ochat-gui/
├── README.md        this document
├── main.c           program entry: opens link, runs screen
├── build.py         OnyxCC build: app + lib/ochat + lib/oc2rpc + otablet toolkit
├── device/
│   └── screen.c     /dev/fb0 + mouse + keyboard loop (OnyxOS only)
├── font/
│   └── font.c       5x7 ASCII font
└── gui/
    ├── gui.h        shared types + module API
    ├── model.c      state: contacts, history, polling, ping, view sync
    ├── actions.c    clicks, keys, send, slash commands
    └── draw.c       paints the 240x112 logical screen
```

Licensed under GPL-3.0-or-later.
