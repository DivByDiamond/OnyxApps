/*
 * session.c - PTY session lifecycle for mux.
 *
 * One session = one kernel PTY pair + a child process whose stdin/stdout/
 * stderr are bound to the slave side. Binding happens in a helper instance
 * of this same binary (`mux --bind /dev/pts/N /bin/osh`), which is spawned
 * normally and then calls fcntl(F_BIND_STDIO) on the slave before exec.
 * That keeps the multiplexer free of fork state and gives every session a
 * clean address space.
 *
 * The parent keeps only the master fd: the master keeps the pair alive,
 * and a child exit shows up as EOF/EPIPE on it.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "mux.h"

/* Kernel bind-stdio fcntl command (OnyxKernel syscall/abi.rs). */
#define F_BIND_STDIO 0x4f01
/* SIGKILL in both Linux and the kernel's signal table. */
#define MUX_SIGKILL 9

struct mux_session g_sessions[MUX_MAX_SESSIONS];

static int find_free(void) {
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        if (!g_sessions[i].used) return i;
    }
    return -1;
}

static void set_size(int fd, int rows, int cols) {
    unsigned short ws[4];
    ws[0] = (unsigned short)(rows > 0 ? rows : 24);
    ws[1] = (unsigned short)(cols > 0 ? cols : 80);
    ws[2] = 0;
    ws[3] = 0;
    _onyx_ioctl(fd, TIOCSWINSZ, (long)ws);
}

/* "/dev/pts/<n>" without relying on snprintf. */
static void pts_path(int n, char *out, size_t cap) {
    const char *pfx = "/dev/pts/";
    size_t i = 0;
    while (pfx[i] && i + 2 < cap) {
        out[i] = pfx[i];
        i++;
    }
    if (n >= 10) out[i++] = (char)('0' + (n / 10) % 10);
    out[i++] = (char)('0' + n % 10);
    out[i] = 0;
}

int mux_session_open(const char *title, const char *prog, int rows, int cols) {
    int idx = find_free();
    if (idx < 0) return -1;

    int master = -1, slave = -1;
    if (pty_open(&master, &slave) != 0) {
        return -1;
    }
    int ptn = 0;
    if (_onyx_ioctl(master, TIOCGPTN, (long)&ptn) != 0) {
        _onyx_close(master);
        _onyx_close(slave);
        return -1;
    }
    char path[16];
    pts_path(ptn, path, sizeof(path));
    /* The helper opens the slave itself; the parent does not need it. */
    _onyx_close(slave);
    set_size(master, rows - 1, cols);

    static char arg0[] = MUX_HELPER;
    static char arg_bind[] = MUX_BIND_ARG;
    char *cargv[5];
    cargv[0] = arg0;
    cargv[1] = arg_bind;
    cargv[2] = path;
    cargv[3] = (char *)prog;
    cargv[4] = 0;
    long pid = _onyx_spawn(MUX_HELPER, cargv, 1);
    if (pid < 0) {
        _onyx_close(master);
        return -1;
    }

    struct mux_session *s = &g_sessions[idx];
    memset(s, 0, sizeof(*s));
    s->used = 1;
    s->master = master;
    s->pid = (int)pid;
    s->alive = 1;
    strncpy(s->title, title, sizeof(s->title) - 1);
    return idx;
}

int mux_session_write(int idx, const char *buf, size_t len) {
    if (idx < 0 || idx >= MUX_MAX_SESSIONS || !g_sessions[idx].used) return -1;
    size_t off = 0;
    while (off < len) {
        long n = _onyx_write_fd((int)g_sessions[idx].master, buf + off, len - off);
        if (n <= 0) return -1;
        off += (size_t)n;
    }
    return (int)off;
}

int mux_session_read(int idx, char *buf, size_t cap) {
    if (idx < 0 || idx >= MUX_MAX_SESSIONS || !g_sessions[idx].used) return -1;
    long n = _onyx_read((int)g_sessions[idx].master, buf, cap);
    if (n <= 0) return (int)n;
    struct mux_session *s = &g_sessions[idx];
    /* Append to the replay buffer, dropping the oldest bytes on overflow:
     * switching back should show recent output even though there is no
     * full screen model (see README). */
    size_t n2 = (size_t)n;
    if (n2 >= MUX_REPLAY) {
        memcpy(s->replay, buf + (n2 - MUX_REPLAY), MUX_REPLAY);
        s->replay_len = MUX_REPLAY;
    } else {
        if (s->replay_len + n2 > MUX_REPLAY) {
            size_t drop = s->replay_len + n2 - MUX_REPLAY;
            memmove(s->replay, s->replay + drop, s->replay_len - drop);
            s->replay_len -= drop;
        }
        memcpy(s->replay + s->replay_len, buf, n2);
        s->replay_len += n2;
    }
    return (int)n;
}

void mux_session_resize_all(int rows, int cols) {
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        if (g_sessions[i].used && g_sessions[i].alive) {
            set_size((int)g_sessions[i].master, rows - 1, cols);
        }
    }
}

void mux_session_reap_all(void) {
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        struct mux_session *s = &g_sessions[i];
        if (!s->used || !s->alive) continue;
        int status = 0;
        long r = _onyx_waitpid(s->pid, &status, 1 /* WNOHANG */);
        if (r != 0) s->alive = 0;
    }
}

void mux_session_kill_all(void) {
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        struct mux_session *s = &g_sessions[i];
        if (!s->used) continue;
        if (s->alive) {
            _onyx_kill(s->pid, MUX_SIGKILL);
            int status = 0;
            _onyx_waitpid(s->pid, &status, 0);
        }
        _onyx_close((int)s->master);
        s->used = 0;
    }
}

void mux_session_replay(int idx) {
    if (idx < 0 || idx >= MUX_MAX_SESSIONS || !g_sessions[idx].used) return;
    struct mux_session *s = &g_sessions[idx];
    _onyx_write(1, MUX_CLEAR, strlen(MUX_CLEAR));
    if (s->replay_len > 0) {
        _onyx_write(1, s->replay, s->replay_len);
    }
}
