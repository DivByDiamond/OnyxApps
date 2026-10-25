/*
 * main.c - mux, a tmux-style multiplexer built on Onyx kernel PTYs.
 *
 * Usage: mux [command]      (default command: /bin/osh)
 *
 * Starts one session with the given command, then multiplexes:
 *   ^a c        new session (same command)
 *   ^a n / ^a p next / previous session
 *   ^a 0..9     select session by number
 *   ^a r        redraw the active session from its replay buffer
 *   ^a ?        show the key help
 *   ^a q        quit (kills every child)
 *
 * Input is forwarded byte-for-byte to the active session's master fd, so
 * the child sees a real terminal; output from every session is polled and
 * the active one is written to fd 1. The kernel allows 4 PTY pairs
 * (PTY_MAX), which caps the session count.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <poll.h>
#include "mux.h"

#define CTRL_A 0x01
#define READ_CHUNK 256


static struct termios g_orig;
static int g_raw = 0;

static void raw_on(void) {
    struct termios t;
    if (tcgetattr(0, &g_orig) != 0) return;
    t = g_orig;
    cfmakeraw_apply(&t);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(0, 0, &t) == 0) g_raw = 1;
}

static void raw_off(void) {
    if (g_raw) {
        tcsetattr(0, 0, &g_orig);
        g_raw = 0;
    }
}

static int session_count(void) {
    int n = 0;
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        if (g_sessions[i].used) n++;
    }
    return n;
}

static int switch_relative(int active, int step) {
    for (int k = 1; k <= MUX_MAX_SESSIONS; k++) {
        int idx = (active + step * k + MUX_MAX_SESSIONS * 8) % MUX_MAX_SESSIONS;
        if (g_sessions[idx].used) return idx;
    }
    return active;
}

/* Helper mode: mux --bind <pts-path> <program>.
 *
 * Binds the slave side to fds 0-2 and execs the session command. Runs in
 * the spawned child, so the multiplexer itself never owns the child's
 * address space or fd table. */
static int bind_and_exec(char **argv) {
    int s = (int)_onyx_open(argv[2], 2 /* O_RDWR */, 0);
    if (s < 0) return 1;
    _onyx_fcntl(s, 0x4f01 /* F_BIND_STDIO */, 0);
    _onyx_fcntl(s, 0x4f01, 1);
    _onyx_fcntl(s, 0x4f01, 2);
    if (s > 2) _onyx_close(s);
    char *cargv[2];
    cargv[0] = argv[3];
    cargv[1] = 0;
    _onyx_exec(argv[3], cargv);
    return 127;
}

/* One ^a command. Returns 1 when the multiplexer should quit. */
static int command(unsigned char c, int *active, int *rows, int *cols,
                   const char *prog) {
    switch (c) {
        case 'c': {
            char title[8];
            int n = session_count() + 1;
            title[0] = 's';
            title[1] = (char)('0' + (n % 10));
            title[2] = 0;
            int idx = mux_session_open(title, prog, *rows, *cols);
            if (idx < 0) {
                mux_help_line();
                return 0;
            }
            *active = idx;
            mux_redraw_active(*active, *rows, *cols);
            return 0;
        }
        case 'n': *active = switch_relative(*active, 1); mux_redraw_active(*active, *rows, *cols); return 0;
        case 'p': *active = switch_relative(*active, -1); mux_redraw_active(*active, *rows, *cols); return 0;
        case 'r': mux_redraw_active(*active, *rows, *cols); return 0;
        case '?': mux_help_line(); mux_status_bar(*rows, *cols, *active); return 0;
        case 'q': return 1;
        default:
            if (c >= '0' && c <= '9') {
                int idx = c - '0';
                if (idx < MUX_MAX_SESSIONS && g_sessions[idx].used) {
                    *active = idx;
                    mux_redraw_active(*active, *rows, *cols);
                }
            }
            return 0;
    }
}

int main(int argc, char **argv) {
    if (argc >= 4 && strcmp(argv[1], MUX_BIND_ARG) == 0) {
        return bind_and_exec(argv);
    }
    const char *prog = (argc > 1) ? argv[1] : "/bin/osh";

    int rows = 24, cols = 80;
    mux_get_size(&rows, &cols);
    raw_on();
    _onyx_write(1, MUX_ALT_ENTER MUX_CLEAR, strlen(MUX_ALT_ENTER MUX_CLEAR));

    if (mux_session_open("s1", prog, rows, cols) < 0) {
        _onyx_write(1, "mux: cannot open /dev/ptmx\r\n", 28);
        raw_off();
        return 1;
    }
    int active = 0;
    mux_status_bar(rows, cols, active);

    int armed = 0;
    int quit = 0;
    while (!quit) {
        struct pollfd pfd[1 + MUX_MAX_SESSIONS];
        int slot_of[1 + MUX_MAX_SESSIONS];
        int n = 0;
        pfd[n].fd = 0;
        pfd[n].events = POLLIN;
        pfd[n].revents = 0;
        slot_of[n] = -1;
        n++;
        for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
            if (!g_sessions[i].used) continue;
            pfd[n].fd = g_sessions[i].master;
            pfd[n].events = POLLIN;
            pfd[n].revents = 0;
            slot_of[n] = i;
            n++;
        }

        long ready = poll(pfd, (unsigned long)n, 50);
        if (ready < 0 && errno != EAGAIN && errno != EINTR) break;

        for (int i = 0; i < n && !quit; i++) {
            if (pfd[i].revents == 0) continue;
            if (slot_of[i] < 0) {
                char buf[READ_CHUNK];
                long got = _onyx_read(0, buf, sizeof(buf));
                if (got <= 0) continue;
                char out[READ_CHUNK];
                size_t o = 0;
                for (long k = 0; k < got; k++) {
                    unsigned char c = (unsigned char)buf[k];
                    if (armed) {
                        armed = 0;
                        if (o > 0) {
                            mux_session_write(active, out, o);
                            o = 0;
                        }
                        if (command(c, &active, &rows, &cols, prog)) {
                            quit = 1;
                            break;
                        }
                        continue;
                    }
                    if (c == CTRL_A) {
                        armed = 1;
                        continue;
                    }
                    out[o++] = (char)c;
                }
                if (o > 0) mux_session_write(active, out, o);
                continue;
            }
            int idx = slot_of[i];
            char buf[READ_CHUNK];
            long got = mux_session_read(idx, buf, sizeof(buf));
            if (got > 0 && idx == active) {
                _onyx_write(1, buf, (size_t)got);
                mux_status_bar(rows, cols, active);
            }
        }

        mux_session_reap_all();

        /* Propagate terminal resizes to every child. */
        int nr = rows, nc = cols;
        mux_get_size(&nr, &nc);
        if (nr != rows || nc != cols) {
            rows = nr;
            cols = nc;
            mux_session_resize_all(rows, cols);
            mux_status_bar(rows, cols, active);
        }
    }

    mux_session_kill_all();
    _onyx_write(1, MUX_CLEAR MUX_ALT_LEAVE, strlen(MUX_CLEAR MUX_ALT_LEAVE));
    raw_off();
    return 0;
}
