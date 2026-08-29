# OnyxApps — userland applications for OnyxOS

Collection of optional userland programs for OnyxOS, written in C and
compiled to `.onx` executables with [OnyxCC](https://github.com/DivByDiamond/OnyxCompiller).

**One repository for all apps**: every app is a self-contained directory
under `apps/`; the CI builds all of them with the same pinned compiler and
publishes `.onx` artifacts — no per-app repo / artifact-sync overhead.

```
┌──────────────────────────────────────────────────────────┐
│  OnyxApps (this repo — optional userland)                │
│   apps/vim   apps/btop   apps/...                        │
├──────────────────────────────────────────────────────────┤
│  OnyxCompiller (onyxcc, libonyxc)   ← build toolchain    │
│  OnyxShell / OnyxKernel / OnyxBoot  ← system (separate)  │
│  OnyxOS (image orchestrator)        ← bundles everything │
└──────────────────────────────────────────────────────────┘
```

## Apps

| App | Source | Description |
|-----|--------|-------------|
| `vim` | [apps/vim](apps/vim) | Modal text editor (vim-inspired), see bindings below |

## Layout rules (project-wide)

- **max 4 files per folder** — one folder = one subsystem;
- **max 200 lines per file** — one responsibility per file;
- KISS / DRY: shared logic lives in exactly one place.

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

Each `apps/<name>/*.c` set compiles to `build/<name>.onx`; run it on
OnyxOS as `vim [file]`.

## Adding an app

1. `mkdir apps/myapp` and put your `*.c` files there (self-contained;
   `onyxcc` links libonyxc automatically, no extra flags needed).
2. Append it to `APPS` in the Makefile if it needs custom flags —
   the wildcard build covers plain apps automatically.
3. CI picks it up: syntax-check + real onyxcc build + `.onx` artifact.

## CI

- **syntax-check** — `gcc -fsyntax-only` against `libonyxc` headers
  (fast feedback, no toolchain build).
- **build** — builds OnyxCC from source, compiles every app into `.onx`,
  validates the `ONX1` magic, uploads artifacts.

## Roadmap / known quirks (inherited from the original vim.c)

- `ZZ`/`ZQ` — the save-and-quit block is unreachable in the original
  dispatcher (the `Z` case returns first and the `g_pending` block clears
  the flag). Preserved verbatim during the split; fix is pending.
- `.` (repeat last change) is a stub message.
- Relative line numbers (`:set rnu`) render but the ruler is shared with
  absolute numbers.
- `N` (reverse search repeat) prints a hint; use `?pattern` instead.

---

vim.c — OnyxOS modal text editor (vim-inspired), single file.

Standard vim binds implemented:

NORMAL mode:
h j k l        move cursor          w b e     word motions
0 ^ $          line start/first-nonspace/end
gg G           file top/bottom      { }       paragraph motions
f<c> F<c> t<c> T<c>  find char on line (with ; and , repeat)
x X            delete char under/before
dd dw d$ d0 dG dgg dj dk   deletes (+ count: 3dd)
cc cw c$ c0    change (delete + insert)
D C            delete to EOL / change to EOL
S s            substitute line / char
yy yw y$ yj yk yank   (+ count)
p P            paste after/before (linewise + charwise)
r<c>           replace char         J         join lines
u U            undo, undo-line      Ctrl+r    redo
.              repeat last change
>> <<          indent/outdent line
~              toggle case of char
i a I A o O    enter INSERT (before/after/line-start/line-end/
open-below/open-above)
v V            VISUAL char/line mode: move extends, d/x/y delete/yank,
>/< indent, ~ case-toggle, Esc exit
/pat<CR>       search forward       ?pat<CR>  search backward
n N            repeat search        *         search word under cursor
ggVG           select all (via visual G)
zz             center cursor line (scroll)
Ctrl+f Ctrl+b  page down/up         Ctrl+d Ctrl+u  half-page
%              bracket match
ZZ             :wq     ZQ           :q!

INSERT mode: printable, Enter, Backspace, Tab(→4 sp), Esc → NORMAL.

COMMAND (:) mode:
:w [file]      write            :q         quit (fails if modified)
:wq / :x       write+quit       :q!        force quit
:e file        open file        :enew      new empty buffer
:r file        insert file below cursor
:set nu / :set nonu     line numbers
:set rnu / :set nornu   relative numbers
:$ :1 :N       go to line N / end
:s/old/new/[g] substitute (current line, g = all occurrences)
:%s/old/new/[g] substitute whole file
:h             help             :ver       version

Run:    vim [file]


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
