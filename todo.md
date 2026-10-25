# OnyxApps — backlog

> Актуально на 2026-10-25. Правила: ≤4 файла на папку, ≤200 строк на файл,
> KISS/DRY/SOLID, C99, самодостаточная папка `apps/<name>/`.
> Статусы: `idea` → `wip` → `done` (в CI-сборке) → `released`.

## Приложения

| App | Статус | Что делает | Что осталось |
|-----|--------|------------|--------------|
| osnake | wip | Змейка в ANSI-терминале (raw termios, non-blocking input, стены/сам себя, ускорение) | Чистая логика (`game/`) покрыта host-тестами; доделать UX/счёт и прогнать в QEMU |
| otop | wip | Монитор системы в стиле btop: реальные данные из `/proc` (meminfo, load, cpuinfo, uptime) + свой heap через sbrk, спарклайн истории heap | Довести раскладку/обновление по таймеру; проверить на OC2R |
| ohttp | done | Минимальный HTTP/1.1 GET (`host[:port]/path` → файл; IPv4 или DNS-имя) | Убрать busy-spin `OHTTP_RECV_RETRIES` (больше не нужен после фикса `usleep`) |
| obrowse | done | TUI-браузер: стадии A–D (fetch → разбор HTML → список ссылок → навигация → overlay ошибок) | Живой сквозной прогон URL → DNS → TCP → GET → render → link → back (P1 N2 в `OnyxKernel/todo.md`) |
| otree | idea | Дерево каталогов (`otree [path]`), рекурсивный readdir + box-drawing | — |
| ocalc | idea | Инфиксный калькулятор `+-*/%` и скобки, `math.h` | — |
| omas | idea | «Матричный дождь» в ANSI, только ASCII-глифы | — |
| obench | idea | Бенчмарк CPU/памяти (целые, FP, memcpy), отчёт в ops/s | — |
| ogrep | idea | Поиск подстроки в файлах, рекурсивный обход, `-i` | — |

## obrowse: честные ограничения ядра

Минимальный текстовый браузер возможен, «настоящий» — нет, пока ядро
(`net_sys`) не умеет больше:

- только исходящий TCP: `connect/send/recv/close`, до 8 соединений;
- `listen`/`accept` — только root-only admin-сокеты, приложению недоступны;
- TLS нет — только HTTP;
- DNS есть в userspace (`net_resolve()`, syscall #89);
- EOF: `tcp_recv` возвращает `Ok(0)` при FIN (стадия C закрыта), но recv
  по-прежнему неблокирующий — приложение само крутит опрос.

## Как добавить приложение

1. `mkdir apps/<name>` с `.c`-файлами (см. правила выше).
2. Добавить `<name>` в оба `matrix:`-списка в `.github/workflows/ci.yml`.
3. CI соберёт `build/<name>.onx` и проверит магию `ONX1`.

## Политика тестирования

- Чистые модули (логика игры, парсеры) — host-тесты в `tests/native/`
  (`make -C tests/native test`), запускаются на любом Linux с gcc.
- CI: gcc-синтаксис против заголовков libonyxc, затем полная сборка через
  onyxcc с проверкой магии `ONX1`.
- Интерактивное поведение — вручную в QEMU (`OnyxOS/scripts/run-qemu.sh`,
  затем `run /bin/<app>`).
