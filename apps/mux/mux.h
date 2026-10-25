/*
 * mux.h - shared state for the Onyx PTY multiplexer.
 *
 * A "session" is one kernel PTY pair (max 4, see OnyxKernel fs/pty) with a
 * child process attached to the slave side. The multiplexer holds the
 * master fds, forwards the active session's I/O to the real terminal and
 * keeps a bounded replay buffer per session so switching back shows the
 * recent output instead of a blank screen.
 *
 * Only the session table is shared state; the window geometry and the
 * active index are passed explicitly. That keeps every UI/loop decision in
 * main.c, where it is easy to reason about (and avoids scalar globals,
 * which this toolchain has been known to mis-address).
 */
#ifndef MUX_H
#define MUX_H

#include <stddef.h>

/* Kernel PTY_MAX. */
#define MUX_MAX_SESSIONS 4
/* Raw output kept per session for redraw on switch (bytes). */
#define MUX_REPLAY 4096
/* argv[0]/flag for the stdio-binding helper instance (see session.c). */
#define MUX_BIND_ARG "--bind"
#define MUX_HELPER "/bin/mux"

/* ANSI helpers. */
#define MUX_ALT_ENTER "\x1b[?1049h"
#define MUX_ALT_LEAVE "\x1b[?1049l"
#define MUX_CLEAR "\x1b[2J\x1b[H"

struct mux_session {
    int  used;
    long master;          /* /dev/ptmx fd token */
    int  pid;             /* helper pid */
    int  alive;
    char title[32];
    char replay[MUX_REPLAY];
    size_t replay_len;
};

extern struct mux_session g_sessions[MUX_MAX_SESSIONS];

/* session.c */
int  mux_session_open(const char *title, const char *prog, int rows, int cols);
int  mux_session_write(int idx, const char *buf, size_t len);
int  mux_session_read(int idx, char *buf, size_t cap);
void mux_session_resize_all(int rows, int cols);
void mux_session_reap_all(void);
void mux_session_kill_all(void);
void mux_session_replay(int idx);

/* ui.c */
void mux_get_size(int *rows, int *cols);
void mux_status_bar(int rows, int cols, int active);
void mux_help_line(void);
void mux_redraw_active(int active, int rows, int cols);

#endif /* MUX_H */
