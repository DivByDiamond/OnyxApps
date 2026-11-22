# OnyxApps / ochat

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Command-line chat and network tools for the OC2R radio network, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS). Talks to
other PCs and robots by their ORA address (`7K3F:02Q9`) over the OC2R RPC
bus (`/dev/hvc0`).

## Build

```sh
make ochat ONYXCC=../OnyxCompiller/onyxcc
# or, manually:
python3 apps/ochat/build.py --compiler ../OnyxCompiller/onyxcc --output build/ochat.onx
```

Runs in the user ring: `/dev/hvc0` opens for uid 0 without ring 1. Pulls in
`lib/ochat` and `lib/oc2rpc` (sources + headers).

## Usage

```sh
ochat send <address|alias> <text>   # send one message (text up to 1024 bytes, UTF-8)
ochat recv [--follow]               # print waiting messages; --follow keeps polling
ochat shell                         # interactive chat
ochat who                           # list devices: address, online/offline, alias
ochat ping <address|alias> [-c N]   # N = 1..100 (default 4); RTT per reply, 3 s timeout
ochat status [address|alias]        # own address, endpoint, security, route estimate
```

Addresses look like `7K3F:02Q9`. Aliases are resolved through the device list;
ambiguous or unknown aliases are rejected.

`recv` consumes messages (the radio queue is emptied when polled). `ping`
exits 0 when at least one reply arrived.

`ochat shell` prompt commands: `/to <address|alias>` choose the peer, `/who`,
`/ping [dest]`, `/history` (last 20 of the 64 kept), `/quit` (or Ctrl-D on an
empty line, Ctrl-C). Any other line is sent to the current peer; the first
sender becomes the peer if none is set. Incoming text is sanitised (invalid
UTF-8 becomes `?`, control characters including ESC are dropped).

## Requirements

- OC2R RPC bus `/dev/hvc0`. Only one program may own it at a time: close
  `oc2ctl`, `ochat-gui` or other `ochat` instances first. Otherwise the first
  call times out (`no reply from the RPC bus`); after a timeout the tool
  refuses further calls (`RPC bus out of sync`) until restarted.
- ORA addresses (`XXXX:XXXX`) provided by the OC2R mod (`gps_tower`).

## Testing

The pure modules live in `lib/ochat` (addresses, framing, history ring, ping
statistics, JSON/wire parsing, line editor, wrapping) and are covered by host
tests that run on any Linux with gcc:

```sh
make -C tests/native test      # includes test_ochat
```

The transport (`lib/ochat/link`) and the RPC bus need a live OnyxOS guest.

## Project structure

```text
apps/ochat/
├── README.md     this document
├── main.c        program entry: command dispatch
├── build.py      OnyxCC build: app + lib/ochat + lib/oc2rpc
└── cli/
    ├── cli.h     shared types + module API
    ├── cmd.c     one-shot commands (send/recv/who/ping/status)
    └── shell.c   interactive line chat (raw terminal)
```

Licensed under GPL-3.0-or-later.
