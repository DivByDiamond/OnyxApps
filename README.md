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

<p align="center">
  <img src="https://img.shields.io/badge/platform-OnyxOS%20%7C%20RISC--V%2064--bit-green" alt="RISC-V 64">
  <img src="https://img.shields.io/badge/language-C99-orange" alt="C99">
  <img src="https://img.shields.io/badge/compiler-OnyxCC-yellow" alt="OnyxCC">
  <img src="https://img.shields.io/badge/license-GPL--3.0-red" alt="GPL-3.0">
</p>

----

OnyxApps is the single repository for every optional userspace program of
[OnyxOS](https://github.com/DivByDiamond/OnyxOS): 11 self-contained
applications, written in C99 and compiled to `.onx` executables by the
project's own self-hosting compiler
([OnyxCC](https://github.com/DivByDiamond/OnyxCompiller)) against
`libonyxc` (stdio / termios / string / stdlib).

One repo for all apps: every program is a directory under `apps/`, the CI
builds all of them with the same pinned compiler and publishes `.onx`
artifacts, so there is no per-app repository and no artifact-sync
overhead. The system components stay in their own repositories; see the
ecosystem table below.

----

## Key features

- **One repo, many apps**: a new program is a directory, not a repository
- **OnyxCC pipeline**: `onyxcc` links `libonyxc` automatically, no extra
  flags
- **Enforced layout**: max 4 files per folder, max 200 lines per file,
  KISS/DRY
- **CI-built artifacts**: every push rebuilds all apps and uploads `.onx`
  files

----

## Apps

| App | Source | Docs | Description |
|-----|--------|------|-------------|
| **vim** | [apps/vim](apps/vim) | [apps/vim/README.md](apps/vim/README.md) | Modal text editor (vim-inspired) |
| **lua** | [apps/lua](apps/lua) | [apps/lua/README.md](apps/lua/README.md) | Standard Lua 5.5.1 interpreter |
| **luac** | [apps/luac](apps/luac) | [apps/luac/README.md](apps/luac/README.md) | Standard Lua 5.5.1 bytecode compiler |
| **mux** | [apps/mux](apps/mux) | [apps/mux/README.md](apps/mux/README.md) | PTY terminal multiplexer |
| **otop** | [apps/otop](apps/otop) | [apps/otop/README.md](apps/otop/README.md) | System monitor reading `/proc` |
| **osnake** | [apps/osnake](apps/osnake) | [apps/osnake/README.md](apps/osnake/README.md) | Snake in the ANSI terminal |
| **obrowse** | [apps/obrowse](apps/obrowse) | [apps/obrowse/README.md](apps/obrowse/README.md) | Text-mode HTTP page viewer (experimental) |
| **jstest** | [apps/jstest](apps/jstest) | [apps/jstest/README.md](apps/jstest/README.md) | `/dev/jsN` joystick event diagnostic |
| **ochat**, **ochat-gui** | [apps/ochat](apps/ochat), [apps/ochat-gui](apps/ochat-gui) | - | OC2R radio chat, CLI and framebuffer GUI |
| **otablet** | [apps/otablet](apps/otablet) | [apps/otablet/README.md](apps/otablet/README.md) | Framebuffer UI toolkit and tablet demo |

Full documentation lives in **each app's own `README.md`**; this file
covers the monorepo only.

----

## Layout rules (project-wide)

- **max 4 files per folder**: one folder = one subsystem
- **max 200 lines per file**: one responsibility per file
- **KISS / DRY**: shared logic lives in exactly one place

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

----

## Build

Requires `onyxcc` (built from OnyxCompiller: `make -C OnyxCompiller`).

```sh
make                                  # build every app into build/
make vim                              # build one app
ONYXCC=/path/to/onyxcc make           # custom compiler path
```

Apps can provide `apps/<name>/build.py` for verified vendored upstream
sources; Make and OnyxOS invoke it with `--compiler` and `--output`. Lua
and luac use this path. Their archives are compiled by the real-build CI
matrix; the C-only syntax-check matrix remains for directly stored C apps.

Each `apps/<name>/` tree compiles to `build/<name>.onx`; run it on
OnyxOS as `vim [file]`.

----

## Adding an app

1. `mkdir apps/myapp` and put your `*.c` files there (self-contained;
   `onyxcc` links libonyxc automatically, no extra flags needed)
2. CI picks it up automatically: syntax-check + real onyxcc build + `.onx`
   artifact. Add the app to the `matrix` lists in
   `.github/workflows/ci.yml`

----

## CI

- **syntax-check**: `gcc -fsyntax-only` against `libonyxc` headers (fast
  feedback, no toolchain build)
- **build**: builds OnyxCC from source, compiles every app into `.onx`,
  validates the `ONX1` magic, uploads artifacts

----

## Ecosystem

| Repository | Role | Language |
|---|---|---|
| [OnyxOS](https://github.com/DivByDiamond/OnyxOS) | Ecosystem: build, disk image, docs | Bash / Python |
| [OnyxKernel](https://github.com/DivByDiamond/OnyxKernel) | RISC-V 64 kernel (Sv39, 3 rings) | Rust |
| [OnyxBoot](https://github.com/DivByDiamond/OnyxBoot) | Bootloader (M-mode, FDT) | C++20 |
| [OnyxShell](https://github.com/DivByDiamond/OnyxShell) | User shell `osh` | Rust |
| [OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) | C99 compiler `onyxcc` + linker | C99 |
| [OnyxApps](https://github.com/DivByDiamond/OnyxApps) | Userland applications | C99 |
| [OnyxDesktop](https://github.com/DivByDiamond/OnyxDesktop) | Desktop environment `onyxdesk` | C99 |

----

## License

Licensed under GPL-3.0-or-later. The vendored Lua distribution retains
its MIT license; see [apps/lua/vendor/LICENSE.txt](apps/lua/vendor/LICENSE.txt).

----

## Русская версия

OnyxApps: единый репозиторий для всех необязательных пользовательских
программ [OnyxOS](https://github.com/DivByDiamond/OnyxOS). 11
самодостаточных приложений написаны на C99 и собираются собственным
компилятором проекта ([OnyxCC](https://github.com/DivByDiamond/OnyxCompiller))
в исполняемые файлы `.onx` с линковкой `libonyxc`
(stdio / termios / string / stdlib).

Один репозиторий на все приложения: каждая программа это каталог в
`apps/`, CI собирает их все одним закреплённым компилятором и публикует
артефакты `.onx`, поэтому нет репозитория на каждое приложение и нет
накладных расходов на синхронизацию артефактов. Системные компоненты живут
в отдельных репозиториях, см. таблицу экосистемы ниже.

----

## Ключевые свойства

- **Один репозиторий, много приложений**: новая программа это каталог, а
  не репозиторий
- **Конвейер OnyxCC**: `onyxcc` линкует `libonyxc` автоматически, без
  дополнительных флагов
- **Принудительная структура**: максимум 4 файла на папку, максимум 200
  строк на файл, KISS/DRY
- **Артефакты из CI**: каждый push пересобирает все приложения и
  выкладывает файлы `.onx`

----

## Приложения

| Приложение | Исходники | Документация | Описание |
|-----|--------|------|-------------|
| **vim** | [apps/vim](apps/vim) | [apps/vim/README.md](apps/vim/README.md) | Модальный текстовый редактор (в духе vim) |
| **lua** | [apps/lua](apps/lua) | [apps/lua/README.md](apps/lua/README.md) | Стандартный интерпретатор Lua 5.5.1 |
| **luac** | [apps/luac](apps/luac) | [apps/luac/README.md](apps/luac/README.md) | Стандартный компилятор байткода Lua 5.5.1 |
| **mux** | [apps/mux](apps/mux) | [apps/mux/README.md](apps/mux/README.md) | Мультиплексор терминалов на PTY |
| **otop** | [apps/otop](apps/otop) | [apps/otop/README.md](apps/otop/README.md) | Монитор системы, читает `/proc` |
| **osnake** | [apps/osnake](apps/osnake) | [apps/osnake/README.md](apps/osnake/README.md) | Змейка в ANSI-терминале |
| **obrowse** | [apps/obrowse](apps/obrowse) | [apps/obrowse/README.md](apps/obrowse/README.md) | Текстовый просмотрщик HTTP-страниц (экспериментальный) |
| **jstest** | [apps/jstest](apps/jstest) | [apps/jstest/README.md](apps/jstest/README.md) | Диагностика событий джойстика `/dev/jsN` |
| **ochat**, **ochat-gui** | [apps/ochat](apps/ochat), [apps/ochat-gui](apps/ochat-gui) | - | Радиочат OC2R: CLI и GUI на фреймбуфере |
| **otablet** | [apps/otablet](apps/otablet) | [apps/otablet/README.md](apps/otablet/README.md) | UI-тулкит на фреймбуфере и демо планшета |

Полная документация лежит в **`README.md` каждого приложения**; этот файл
описывает только монорепозиторий.

----

## Правила структуры (общие для проекта)

- **максимум 4 файла на папку**: одна папка = одна подсистема
- **максимум 200 строк на файл**: одна ответственность на файл
- **KISS / DRY**: общая логика живёт ровно в одном месте

Эталонная структура (`apps/vim`):

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

----

## Сборка

Требуется `onyxcc` (собирается из OnyxCompiller:
`make -C OnyxCompiller`).

```sh
make                                  # собрать все приложения в build/
make vim                              # собрать одно приложение
ONYXCC=/path/to/onyxcc make           # свой путь к компилятору
```

Приложение может предоставить `apps/<name>/build.py` для верифицированных
вендорских исходников; Make и OnyxOS вызывают его с аргументами
`--compiler` и `--output`. Lua и luac используют этот путь. Их архивы
собирает матрица real-build CI; матрица syntax-check для чистого C
остаётся для приложений, чьи исходники лежат в репозитории напрямую.

Каждое дерево `apps/<name>/` собирается в `build/<name>.onx`; на OnyxOS
запускайте как `vim [файл]`.

----

## Добавление приложения

1. `mkdir apps/myapp` и положите туда файлы `*.c` (самодостаточно;
   `onyxcc` линкует libonyxc автоматически, дополнительные флаги не нужны)
2. CI подхватит приложение автоматически: syntax-check + реальная сборка
   onyxcc + артефакт `.onx`. Добавьте приложение в списки `matrix` в
   `.github/workflows/ci.yml`

----

## CI

- **syntax-check**: `gcc -fsyntax-only` по заголовкам `libonyxc` (быстрая
  обратная связь, без сборки тулчейна)
- **build**: собирает OnyxCC из исходников, компилирует каждое приложение
  в `.onx`, проверяет магию `ONX1`, выкладывает артефакты

----

## Экосистема

| Repository | Role | Language |
|---|---|---|
| [OnyxOS](https://github.com/DivByDiamond/OnyxOS) | Ecosystem: build, disk image, docs | Bash / Python |
| [OnyxKernel](https://github.com/DivByDiamond/OnyxKernel) | RISC-V 64 kernel (Sv39, 3 rings) | Rust |
| [OnyxBoot](https://github.com/DivByDiamond/OnyxBoot) | Bootloader (M-mode, FDT) | C++20 |
| [OnyxShell](https://github.com/DivByDiamond/OnyxShell) | User shell `osh` | Rust |
| [OnyxCompiller](https://github.com/DivByDiamond/OnyxCompiller) | C99 compiler `onyxcc` + linker | C99 |
| [OnyxApps](https://github.com/DivByDiamond/OnyxApps) | Userland applications | C99 |
| [OnyxDesktop](https://github.com/DivByDiamond/OnyxDesktop) | Desktop environment `onyxdesk` | C99 |

----

## Лицензия

Лицензия GPL-3.0-or-later. Вендорский дистрибутив Lua сохраняет лицензию
MIT; см. [apps/lua/vendor/LICENSE.txt](apps/lua/vendor/LICENSE.txt).
