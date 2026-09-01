# OnyxApps - TODO / roadmap of new apps

Statuses: `idea` -> `wip` -> `done` -> `released` (in a CI build).

Rules for every app (project-wide, see main README):
max 4 files per folder, max 200 lines per file, KISS / DRY / SOLID,
C99, self-contained folder under `apps/`, one responsibility per file.

## Backlog

| App     | Status | Description | Notes |
|---------|--------|-------------|-------|
| osnake  | wip    | Classic snake game on the ANSI terminal (raw termios, non-blocking input, wall/self collision, auto speed-up) | `game/` pure logic is host-testable |
| otop    | wip    | btop-style system monitor for OnyxOS: reads real data from `/proc` (meminfo, load, cpuinfo, uptime) + own heap via sbrk, heap history sparkline | unlike osysmon shows kernel-provided values, not synthetic bars |
| otree   | idea   | Directory tree renderer (`otree [path]`), recursive readdir + box-drawing ASCII | small, one evening |
| ocalc   | idea   | Infix calculator with +-*/% and parentheses, `math.h` for sqrt/pow | parser ~150 lines, keep it pure and test it |
| omas    | idea   | "Matrix rain" ANSI screensaver, pure ASCII glyphs | fun demo for screenshots |
| obench  | idea   | CPU / memory benchmark: integer and FP loops, memcpy bandwidth, reports in ops/s | useful on real RISC-V boards (Milk-V DuoS) |
| ogrep   | idea   | Substring search in files, recursive walk via readdir, `-i` case-insensitive | closes the biggest CLI gap after editors |
| obrowse | idea   | TUI web browser (experimental, see below) | staged plan |

## Done

(none yet in this batch - osnake and otop land with this commit series)

## obrowse - TUI browser: how hard is it?

Honest answer: a *minimal* text browser is doable, a "real" one is not,
because of kernel limits (OnyxKernel `net_sys`):

- only outbound TCP: `connect/send/recv/close`, max 8 connections
- no `listen/accept`, no TLS, HTTP only
- DNS is not exposed to userland: net syscalls take a raw IPv4 address,
  so either add a DNS resolver syscall to the kernel or use a hosts-file

Staged plan:

1. `ohttp` - fetch `http://IP:PORT/PATH` and save the body to a file
   (pure app work, no kernel changes). This is also step 1 for obrowse.
2. `obrowse` stage A - render a fetched HTML page as text in a TUI
   (strip tags, keep link list `[1] text`, scroll with PgUp/PgDn).
3. `obrowse` stage B - follow links: resolve relative URLs, fetch, redraw.
4. `obrowse` stage C (needs kernel patches, separate repo):
   DNS resolution syscall and bigger recv buffers.

## How to add an app

1. `mkdir apps/<name>` with the `.c` files (see layout rules above)
2. add `<name>` to both `matrix:` lists in `.github/workflows/ci.yml`
3. CI builds it into `build/<name>.onx` and validates the `ONX1` magic

## Testing policy

- pure modules (game logic, parsers) get host tests in `tests/native/`
  (`make -C tests/native test`), runnable on any Linux with gcc
- CI does the real check: gcc syntax pass against libonyxc headers, then
  a full onyxcc build with `ONX1` magic validation
- interactive behavior is verified manually in QEMU
  (`OnyxOS/scripts/run-qemu.sh`, then `run /bin/osnake`)
