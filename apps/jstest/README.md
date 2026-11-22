# OnyxApps / jstest

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Joystick diagnostic for OnyxOS, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS). Prints
every event from a `/dev/jsN` joydev-compatible node, one line each:
the open-time init burst (axis baselines, button states) and then live
axis/button events.

## Build

```sh
make jstest                # from the repo root (onyxcc in PATH or ONYXCC=...)
```

## Usage

```sh
jstest               # read /dev/js0, the first joystick
jstest /dev/js1      # read a specific node
jstest -t 10         # exit 0 after 10 seconds (scripted runs, smoke test)
```

Output lines (init records from the open-time burst are marked):

```text
axis 0 = 0 (init)
axis 1 = -1234
button 5 down
button 5 up (init)
```

Exit codes: `0` clean exit (`--help` or the `-t SECS` deadline; without
`-t` the event loop never ends), `2` usage error, `3` device error (open failure or a
non-EAGAIN read). An empty event ring makes `read()` fail with
`EAGAIN`; the loop then sleeps 10 ms and retries.

## Requirements

Works against the kernel's USB HID joystick driver: `/dev/jsN`
devfs nodes serving 8-byte joydev-compatible records (u32 time_ms LE,
i16 value LE, u8 type, u8 number; type 0x01 button, 0x02 axis,
0x80 OR'ed in for init records) - provided by OnyxKernel
(`kernel/src/fs/devfs/js.rs`).

## Testing

The record decode and line formatting (`events.c`) are pure C99 and
are covered by host tests that run on any Linux with gcc:

```sh
make -C tests/native test      # includes test_jstest
```

Covered: the exact wire decode (little-endian time/value, i16 sign),
button and axis records, the init bit in both, the one-line output
format, rejection of unknown type bits. The live read loop (open,
EAGAIN retry) is verified live by `OnyxOS/scripts/qemu-jstest-smoke.sh`
(QEMU xHCI + usb-tablet, input injected over QMP).

## Project structure

```text
apps/jstest/
├── README.md     this document
├── jstest.h      shared types (struct js_event) + module API
├── events.c      pure record decode + one-line formatting (host-tested)
└── main.c        program entry: args, open, EAGAIN retry read loop
```

Licensed under GPL-3.0-or-later.
