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
- **test_ochat** - `lib/ochat/{text,data,wire}`: ORA address validation
  and alias resolution, text framing (UTF-8 validation, control/escape
  stripping, 1024-byte bound), UUID text, history ring overflow, ping
  statistics, wrapping, the bounded JSON scanner (malformed/deep input)
  and radio RPC reply parsing/parameter building. Run under
  ASan/UBSan during development.
- **test_ochat_gui** - `apps/ochat-gui` model, actions and drawing against
  a fake radio backend (contacts, unread, send errors, ping states,
  slash commands, extreme histories).

## What is not covered here

Terminal I/O (raw termios, ANSI drawing) and syscall probes need a live
OnyxOS: verify those manually in QEMU (`OnyxOS/scripts/run-qemu.sh`,
then `run /bin/<app>`) and rely on CI for the real onyxcc build with
`ONX1` magic validation.

## Rules

- only pure modules are linked here; if a module needs I/O to be
  testable, split the logic out of it (SOLID) until the logic part is
  pure
- one test driver per module (test_<name>.c); keep this README current
