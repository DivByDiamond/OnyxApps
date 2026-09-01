# OnyxApps / tests/native

Host-side tests for the pure modules of the apps in this repo. They run
on any Linux with gcc; no OnyxOS, onyxcc or RISC-V toolchain required.

```sh
make test          # build and run every test
make test_osnake   # one test only
make clean
```

## What is covered

- **test_osnake** - `apps/osnake/game/` (pure C99, no I/O):
  reset invariants, 180-degree reversal rejection, wall death,
  self-collision death, food growth and scoring, full-board food spawn
  failure (the win path), LCG determinism, plus a 30x100-step fuzz run
  that asserts length/score/bounds invariants after every step.
- **test_otop** - `apps/otop/probe/parse.c` (pure C99, no I/O):
  `/proc/meminfo`, `/proc/load`, `/proc/cpuinfo` field parsing against
  the exact kernel formats, uptime parsing, permille edge cases (zero
  total, overflow clamp), `HH:MM:SS` formatting, history ring shifting,
  sparkline output (levels, padding, scaling).

## What is not covered here

Terminal I/O (raw termios, ANSI drawing) and syscall probes need a live
OnyxOS: verify those manually in QEMU (`OnyxOS/scripts/run-qemu.sh`,
then `run /bin/<app>`) and rely on CI for the real onyxcc build with
`ONX1` magic validation.

## Rules

- only pure modules are linked here; if a module needs I/O to be
  testable, split the logic out of it (SOLID) until the logic part is
  pure
- keep this folder at max 4 files: the Makefile, this README and the
  two test drivers
