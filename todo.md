# OnyxApps — backlog

> Актуально на 2026-10-25. Правила: ≤4 файла на папку, ≤200 строк на файл,
> KISS/DRY/SOLID, C99, самодостаточная папка `apps/<name>/`.
> Статусы: `idea` → `wip` → `done` (в CI-сборке) → `released`.

## Приложения

| App | Статус | Что делает | Что осталось |
|-----|--------|------------|--------------|
| osnake | wip | Змейка в ANSI-терминале (raw termios, non-blocking input, стены/сам себя, ускорение) | Чистая логика (`game/`) покрыта host-тестами; доделать UX/счёт и прогнать в QEMU |
| otop | wip | Монитор системы в стиле btop: реальные данные из `/proc` (meminfo, load, cpuinfo, uptime) + свой heap через sbrk, спарклайн истории heap | Довести раскладку/обновление по таймеру; проверить на OC2R |
| ochat | wip | CLI-чат по радио OC2R (ORA-адреса): send/recv/shell/who/ping/status через RPC-шину `/dev/hvc0` (`lib/ochat`, `lib/oc2rpc`) | Живой прогон с модом; подтвердить формат ответов (`OnyxOS/docs/dev/ochat.md`), `listRadioDevices` |
| ochat-gui | wip | GUI-чат (framebuffer+мышь, тулкит otablet): список устройств, сообщения, ввод, индикатор ping | Живой прогон в QEMU; логика покрыта host-тестами |
| obrowse | done | TUI-браузер: стадии A–D (fetch → разбор HTML → список ссылок → навигация → overlay ошибок) | Живой сквозной прогон URL → DNS → TCP → GET → render → link → back (P1 N2 в `OnyxKernel/todo.md`) |
| ofetch | idea | Аналог fastfetch: ASCII-лого Onyx + версия ядра, uptime, память, CPU, hart'ы, разрешение фреймбуфера из `/proc` и `/dev/fb0` | — |
| otree | idea | Дерево каталогов (`otree [path]`), рекурсивный readdir + box-drawing | — |
| ocalc | idea | Инфиксный калькулятор `+-*/%` и скобки, `math.h` | — |
| omas | idea | «Матричный дождь» в ANSI, только ASCII-глифы | — |
| obench | idea | Бенчмарк CPU/памяти (целые, FP, memcpy), отчёт в ops/s | — |
| ogrep | idea | Поиск подстроки в файлах, рекурсивный обход, `-i` | — |

## Чистка и дедупликация

- **Вынести в `lib/` общий слой терминала** (`lib/term`): raw-termios (вход/выход из raw, восстановление), разбор escape-последовательностей клавиш, запрос размера через `TIOCGWINSZ`. Сейчас это скопировано в vim, osnake, otop, obrowse, mux, ochat и ochat-gui; после выноса каждое приложение подключает общий код, правки делаются в одном месте.
- **Вынести HTTP-клиент в `lib/`** (`lib/http`): сейчас он живёт только внутри `obrowse/net/fetch.c`; когда понадобится второй потребитель (например, `ofetch` или загрузчик), брать оттуда, а не писать заново.
- Убрано: `dnstest` (отладочная заглушка на 17 строк), `ohttp` (дублировал HTTP-клиент obrowse, остался `obrowse/net/fetch.c`), `oed` (остался `vim`).

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
