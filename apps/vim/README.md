# OnyxApps / vim

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

Modal text editor inspired by Vim, part of
[OnyxApps](https://github.com/DivByDiamond/OnyxApps) - the userland apps
monorepo for [OnyxOS](https://github.com/DivByDiamond/OnyxOS). Implements
the familiar Vim editing model: modes, registers, counts, search,
substitution and a `:` command line.

----

Modal text editor inspired by Vim. Implements the familiar Vim editing
model: modes, registers, counts, search, substitution and a `:` command
line.

## Modes

| Mode | Enter | Indicator |
|------|-------|-----------|
| NORMAL | Esc | `[NORMAL]` |
| INSERT | `i a I A o O` | `[INSERT]` |
| VISUAL / V-LINE | `v` / `V` | `[VISUAL]` / `[V-LINE]` |
| COMMAND | `:` | `:cmd` at bottom |
| SEARCH | `/` `?` | `/pat` at bottom |

## Key bindings

### NORMAL mode

- **Motions** - `h j k l`, word motions `w b e`, `0 ^ $`, `gg G`, `{ }`,
  `f/F/t/T` with `; ,` repeat, `%` bracket matching, `zz` center,
  `Ctrl+f/b/d/u` paging, `gg` = file top, `G` / `N G` = line N
- **Editing operators** - `x X dd dw d$ d0 dG dj dk D J cc cw c$ C S s
  r ~ >> <<` (with counts: `3dd`, `2x`)
- **Registers** - yank/paste: `yy yw y$ yj yk`, `p P` (linewise and charwise)
- **Undo / redo** - `u`, `Ctrl+r` (charwise, linewise and inserted text)
- **Search** - `/pat` forward, `?pat` backward, `n` repeat, `*` word under cursor
- **Visual mode** - charwise (`v`) and linewise (`V`): select, delete,
  yank, indent, toggle case, `ggVG` select all

### INSERT mode

Printable characters, Enter, Backspace, Tab (4 spaces), Esc to NORMAL.

### COMMAND (`:`) mode

- `:w [file]` write - `:q` quit (fails if modified) - `:wq` / `:x` write+quit -
  `:q!` force quit
- `:e file` open file - `:enew` new empty buffer - `:r file` insert file below
- `:set nu / nonu` line numbers - `:set rnu / nornu` relative numbers
- `:$` / `:1` / `:N` go to line N or end
- `:s/old/new/[g]` substitute (current line) - `:%s/old/new/[g]` whole file
- `:h` help - `:ver` version
- `ZZ` = `:wq`, `ZQ` = `:q!`

## Installing into OnyxOS

1. Build the boot disk as usual (`scripts/build-all.sh` in OnyxOS)
2. Take `vim.onx` from the CI artifacts (or `make vim`) and add a line to
   the disk manifest:
   ```
   /path/to/vim.onx -> /bin/vim
   ```
3. Rebuild the image: `bash scripts/mk-onyxfs-disk.sh`
4. In the system: `run /bin/vim file.txt`

## Testing

Verified with integration runs through the `onx-run` emulator
(compile - run - compare result):

- motion `hjkl w b $ gg G`, counts `3x 2dd 5j`
- edits `x dd dw D J A i o O ~ >> <<`
- registers `yy p P` (linewise and charwise)
- undo/redo `u Ctrl+r`
- ex commands `:w :q :wq :q! :e :r :N :$ :set nu :s/// :%s///g`
- search `/pat n *`
- visual mode `Vjd`, `Vy`

Requires a kernel with an ANSI/VT100 console and termios
(TCGETS/TCSETS, TIOCGWINSZ) - provided by OnyxKernel v0.4+.

## Known quirks

- `ZZ`/`ZQ` - the save-and-quit block is unreachable in the original
  dispatcher (the `Z` case returns first and the `g_pending` block clears
  the flag). Preserved verbatim during the split; fix is pending
- `.` (repeat last change) is a stub message
- `N` (reverse search repeat) prints a hint; use `?pattern` instead

----

## Building

```sh
make vim                    # from the repo root (onyxcc in PATH or ONYXCC=...)
onyxcc -I apps/vim -o vim.onx apps/vim/*.c apps/vim/*/*.c   # manual
```

`libonyxc` (stdio/termios/string/stdlib) is picked up automatically.

## Project structure

See the layout map in the [main README](https://github.com/DivByDiamond/OnyxApps#layout-rules-project-wide).

Licensed under GPL-3.0-or-later.
