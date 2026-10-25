# mux — PTY multiplexer

`mux` is a small tmux-style multiplexer built on the kernel's PTY pairs
(`OnyxKernel/kernel/src/fs/pty`, `/dev/ptmx` + `/dev/pts/N`).

```
mux              # one session running /bin/osh
mux /bin/osh     # explicit command
```

| Key | Action |
|-----|--------|
| `^a c` | new session (kernel limit: 4 PTY pairs) |
| `^a n` / `^a p` | next / previous session |
| `^a 0`…`^a 9` | select session by number |
| `^a r` | redraw the active session from its replay buffer |
| `^a ?` | show the key help |
| `^a q` | quit (kills every child) |

## How it works

* One session = one PTY pair. The multiplexer keeps the **master** fd; the
  **slave** is handed to the child.
* Children are started through `spawn("/bin/mux", ["--bind", "/dev/pts/N",
  <cmd>])`. The helper instance opens the slave, binds it to fds 0–2 with
  `fcntl(F_BIND_STDIO)` and `exec`s the session command. Keeping binding in
  a separate process means the multiplexer never owns the child's fd table.
* Input is forwarded byte-for-byte to the active master; output from every
  master is polled (`poll(2)`) and the active one is written to fd 1.
* The last terminal line is a status bar; the child gets `rows - 1` lines
  via `TIOCSWINSZ`, re-applied whenever the terminal is resized.
* Switching sessions replays the last 4 KiB of that session's raw output
  (`^a r` forces it).

## Limits (deliberate)

* **No terminal emulator.** There is no per-session screen model, so a
  switch replays recent output instead of restoring the exact screen.
  Full-screen programs (vim, otop) redraw only what they write next.
  Split panes need a real screen model and are not implemented yet.
* 4 sessions, matching `PTY_MAX`.
* `mux` is packaged in root space (ring 1) because it execs the session
  shell and owns PTY masters; ordinary ring-2 users cannot run it.
