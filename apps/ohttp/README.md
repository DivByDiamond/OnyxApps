# ohttp

Minimal HTTP/1.1 GET client for OnyxOS. First step toward `obrowse`
(see `../../todo.md`): fetches one URL to a local file over plain TCP.

## Usage

```
ohttp <ip[:port]/path> <outfile>
ohttp 93.184.216.34/index.html page.html
ohttp 10.0.2.2:8080/api/data.json data.json
```

Prints the response status line and `Content-Length` (if present) to
stdout, and writes the body to `<outfile>`.

## Why an IP, not a hostname

OnyxKernel does not expose DNS resolution to userspace yet (kernel-side
DHCP/DNS lease exists, see `OnyxKernel/todo.md`, but there is no
`getaddrinfo`-equivalent syscall). Until that lands, `ohttp` takes a raw
IPv4 the same way `curl --resolve` sidesteps DNS, and sends a `Host:`
header built from that same IP so virtual-hosted servers still see
*something* — a real hostname needs the DNS syscall first.

`https://` targets are rejected up front: the kernel's TCP stack has no
TLS underneath it.

## Layout

- `ohttp.h` — shared types (`ohttp_target`) and module API
- `net/request.c` — parses the `ip[:port]/path` CLI argument
- `net/connect.c` — opens the TCP connection, builds and sends the GET
- `http/response.c` — reads the response, splits headers/body, streams
  the body to a file

## Kernel quirks this app works around

`net_recv` (`OnyxKernel/kernel/src/syscall/net_sys.rs` →
`net::tcp_recv`) has no "peer closed" signal distinct from "no data
yet": both come back as `-1/ENOENT` (it never returns `0`), and the
connection slot only disappears (`-1/EINVAL`) after TIMEWAIT expires,
well after the response has actually finished. `ohttp` therefore trusts
the `Content-Length` header to know when the body is complete, and only
falls back to "read until the slot is gone" for the rare response
without one.

`usleep()`/`nanosleep()` hangs forever on the first call (confirmed
live, see `OnyxKernel/todo.md`) — the retry loop in `http/response.c`
busy-spins between `net_recv` attempts instead of sleeping.

Its own 8KB header buffer is `static`, not a stack local: the initial
user stack pointer sits only a couple KB below `USER_TOP`
(`OnyxKernel/kernel/src/syscall/handler/dispatch.rs`), so an 8KB stack
array pushes `buf + len` past `USER_TOP` and every `net_recv` into it
fails with `EINVAL` even though the memory is actually mapped.

## Live-tested, one bug still blocking full runs

Verified end-to-end in QEMU (2026-09-01): connects, sends the GET, and
correctly prints the response status line and `Content-Length` from a
real server. Writing the body to `<outfile>` is currently unreliable —
`argv[2]` is a string the loader also places near the top of the stack,
so it can hit the same `USER_TOP` proximity issue described above
(`sys_open: parse_user_path failed`). That one is a loader/ABI issue,
not something `ohttp` can work around itself; see `OnyxKernel/todo.md`.

## Not done

- Redirects (3xx) — printed as the status line, not followed
- Chunked transfer-encoding
- DNS / hostnames (blocked on the kernel syscall)
- HTTPS (blocked on kernel TLS, if it ever gets one)
