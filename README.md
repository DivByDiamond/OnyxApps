[![OnyxApps CI](https://github.com/DivByDiamond/OnyxApps/actions/workflows/ci.yml/badge.svg)](https://github.com/DivByDiamond/OnyxApps/actions/workflows/ci.yml)

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>


<p align="center">
<pre class="not-prose" style="text-align:center;font-family:monospace;">
    ███████
  ███▒▒▒▒▒███
 ███     ▒▒███ ████████   █████ ████ █████ █████
▒███      ▒███▒▒███▒▒███ ▒▒███ ▒███ ▒▒███ ▒▒███
▒███      ▒███ ▒███ ▒███  ▒███ ▒███  ▒▒▒█████▒
▒▒███     ███  ▒███ ▒███  ▒███ ▒███   ███▒▒▒███
 ▒▒▒███████▒   ████ █████ ▒▒███████  █████ █████
   ▒▒▒▒▒▒▒    ▒▒▒▒ ▒▒▒▒▒   ▒▒▒▒▒███ ▒▒▒▒▒ ▒▒▒▒▒
                           ███ ▒███
                          ▒▒██████
                           ▒▒▒▒▒▒
 ████████
██▒▒▒▒▒▒███ ████ █████  █████████  █████████
███▒▒▒▒▒███▒▒███ ▒▒███ ▒██▒▒▒▒▒▒███▒▒███▒▒▒▒███
███████████ ▒███  ▒███ ▒██▒▒▒▒▒▒███▒██████████
██▒▒▒▒▒▒███ █████ █████▒██▒▒▒▒▒▒███ ▒▒███████
██▒▒▒▒▒▒███▒███▒▒▒ ▒███ ▒██▒▒▒▒▒▒███  ████████
███▒▒▒▒▒███▒███▒▒▒ ▒███ ███████████▒███▒▒▒▒▒
███▒▒▒▒▒███▒███▒▒▒ ▒███ ▒███▒▒▒▒▒▒ ▒███▒▒▒▒▒
 ▒▒███████▒ ▒▒███████▒  ▒▒█████████  ▒▒███████
  ▒▒▒▒▒▒▒    ▒▒▒▒▒▒▒    ▒▒▒▒▒▒▒▒▒    ▒▒▒▒▒▒▒
</pre>
</p>

<p align="center"><em>Optional userland applications for OnyxOS, written in C99</em></p>

----

OnyxApps is the single repository for every optional userspace program of
[OnyxOS](https://github.com/DivByDiamond/OnyxOS), written in C99 and compiled
to `.onx` executables by the project's own self-hosting compiler
([OnyxCC](https://github.com/DivByDiamond/OnyxCompiller)) against `libonyxc`
(stdio / termios / string / stdlib).

One repo for all apps: every program is a self-contained directory under
`apps/`, the CI builds all of them with the same pinned compiler and
publishes `.onx` artifacts, so there is no per-app repository and no
artifact-sync overhead. The system components stay in their own repos:
[OnyxKernel](https://github.com/DivByDiamond/OnyxKernel),
[OnyxBoot](https://github.com/DivByDiamond/OnyxBoot),
[OnyxShell](https://github.com/DivByDiamond/OnyxShell),
[OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) and the
[OnyxOS](https://github.com/DivByDiamond/OnyxOS) image orchestrator.

----

## Key Features

- **One repo, many apps** - a new program is a directory, not a repository
- **OnyxCC pipeline** - `onyxcc` links `libonyxc` automatically, no extra flags
- **Enforced layout** - max 4 files per folder, max 200 lines per file, KISS/DRY
- **CI-built artifacts** - every push rebuilds all apps and uploads `.onx` files

## Apps

| App | Source | Docs | Description |
|-----|--------|------|-------------|
| **vim** | [apps/vim](apps/vim) | [apps/vim/README.md](apps/vim/README.md) | Modal text editor (vim-inspired) |
| **oed** | [apps/oed](apps/oed) | [apps/oed/README.md](apps/oed/README.md) | Full-screen text editor (nano-style) |
| **osysmon** | [apps/osysmon](apps/osysmon) | [apps/osysmon/README.md](apps/osysmon/README.md) | System monitor (btop/htop-style) |

Full documentation lives in **each app's own `README.md`**; this file covers
the monorepo only.

## Layout rules (project-wide)

- **max 4 files per folder** - one folder = one subsystem
- **max 200 lines per file** - one responsibility per file
- **KISS / DRY** - shared logic lives in exactly one place

Reference structure (`apps/vim`):

```
apps/vim/
├── vim.h            shared state (extern) + full API
├── main.c           program entry: args + editor loop
├── core/            state.c  buffer.c  undo.c  fileio.c
│                    (buffer state, edit primitives, undo/redo, load/save)
├── ops/             registers.c  operators.c
│                    (yank/paste registers, d/c/y operators, indent)
├── text/            motions.c  search.c
│                    (word motions, %, pattern search, f/F/t/T)
├── mode/            keys.c  keys_motion.c  keys_edit.c  command.c
│                    (NORMAL dispatcher, motion keys, edit keys, : commands)
└── ui/              render.c  input.c
                     (drawing, cursor, raw term, key decoding)
```

## Build

Requires `onyxcc` (built from OnyxCompiller: `make -C OnyxCompiller`).

```sh
make                                  # build every app into build/
make vim                              # build one app
ONYXCC=/path/to/onyxcc make           # custom compiler path
```

Each `apps/<name>/` tree compiles to `build/<name>.onx`; run it on
OnyxOS as `vim [file]`.

## Adding an app

1. `mkdir apps/myapp` and put your `*.c` files there (self-contained;
   `onyxcc` links libonyxc automatically, no extra flags needed)
2. CI picks it up automatically: syntax-check + real onyxcc build + `.onx`
   artifact. Add the app to the `matrix` lists in `.github/workflows/ci.yml`

## CI

- **syntax-check** - `gcc -fsyntax-only` against `libonyxc` headers
  (fast feedback, no toolchain build)
- **build** - builds OnyxCC from source, compiles every app into `.onx`,
  validates the `ONX1` magic, uploads artifacts

----

----

## Related Projects

| Project | Description |
|---------|-------------|
| [OnyxOS](https://github.com/DivByDiamond/OnyxOS) | RISC-V operating system (kernel + shell + bootloader + compiler) |
| [OnyxKernel](https://github.com/DivByDiamond/OnyxKernel) | RISC-V 64-bit kernel for OnyxOS |
| [OnyxShell](https://github.com/DivByDiamond/OnyxShell) | POSIX-like shell for OnyxOS |
| [OnyxBoot](https://github.com/DivByDiamond/OnyxBoot) | RISC-V bootloader for OnyxOS |
| [OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) | Self-hosting C compiler used to build the apps |

----

Licensed under GPL-3.0-or-later.
