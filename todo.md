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
| ohttp   | wip    | Minimal HTTP/1.1 GET client (`ip[:port]/path` -> file), obrowse stage 1 | fetches and prints headers successfully end-to-end in QEMU; writing the body still blocked on a kernel bug, see below |
| otree   | idea   | Directory tree renderer (`otree [path]`), recursive readdir + box-drawing ASCII | small, one evening |
| ocalc   | idea   | Infix calculator with +-*/% and parentheses, `math.h` for sqrt/pow | parser ~150 lines, keep it pure and test it |
| omas    | idea   | "Matrix rain" ANSI screensaver, pure ASCII glyphs | fun demo for screenshots |
| obench  | idea   | CPU / memory benchmark: integer and FP loops, memcpy bandwidth, reports in ops/s | useful on real RISC-V boards (Milk-V DuoS) |
| ogrep   | idea   | Substring search in files, recursive walk via readdir, `-i` case-insensitive | closes the biggest CLI gap after editors |
| obrowse | idea   | TUI web browser (experimental, see below) | staged plan, needs ohttp first |

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
   (pure app work, no kernel changes). App-side work **done** (wip, see
   `apps/ohttp/`): takes a raw IPv4 target (no DNS yet), sends a GET,
   trusts `Content-Length` to know when the body is finished (net_recv
   has no clean EOF signal, see `apps/ohttp/README.md` for why).
   libonyxc got `net_connect/send/recv/close` userspace wrappers
   (`io/net.h`) and the missing `SYS_net_*` #80-83 defines to support it.

   **Live-tested end-to-end in QEMU (2026-09-01, `-netdev user` +
   guestfwd to a local HTTP server) and it works** — connects, sends the
   GET, and prints `HTTP/1.0 200 OK` / `Content-Length: 40` from a real
   TCP round trip (DHCP lease acquired, full handshake, data, FIN). This
   took three separate kernel-side bugs to get here, all in
   `OnyxKernel/todo.md`:
   - errno codes were never translated to POSIX values (fixed)
   - virtio-net had no separate TX virtqueue, and only spoke the modern
     virtio-mmio register layout while real QEMU defaults to legacy
     (both fixed — confirmed via pcap, 0 packets before, full DHCP/ARP/TCP
     traffic after)
   - UDP and TCP RX handlers read source/destination port fields
     backwards, so DHCP replies and TCP ACKs never matched a live
     socket/connection (fixed)

   Two more bugs surfaced while chasing this, both **not fixed, not
   ohttp-specific**:
   - `usleep()`/`nanosleep()` hangs forever on the first call — `ohttp`
     works around it with a busy-spin instead of a real sleep
     (`http/response.c`, see the comment on `OHTTP_RECV_RETRIES`)
   - the initial user stack pointer sits only a couple KB below
     `USER_TOP`, so `argv` strings (which the loader also puts near the
     top of the stack) can fail the kernel's `user_ptr_ok` check —
     `ohttp` currently can't reliably open its own `argv[2]` output path
     for writing (`sys_open: parse_user_path failed`). Moved `ohttp`'s
     own 8KB header buffer off the stack (`static`) to dodge the same
     class of bug for its own reads, but the `argv` case is a loader/ABI
     issue outside app control.
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
