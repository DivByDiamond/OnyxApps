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

int main(int argc, char **argv) {
    if (argc >= 4 && strcmp(argv[1], MUX_BIND_ARG) == 0) {
        return mux_bind_and_exec(argv);
    }
    const char *prog = (argc > 1) ? argv[1] : "/bin/osh";

    int rows = 24, cols = 80;
    mux_get_size(&rows, &cols);
    mux_raw_on();
    _onyx_write(1, MUX_ALT_ENTER MUX_CLEAR, strlen(MUX_ALT_ENTER MUX_CLEAR));

    if (mux_session_open("s1", prog, rows, cols) < 0) {
        _onyx_write(1, "mux: cannot open /dev/ptmx\r\n", 28);
        mux_raw_off();
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
                        if (mux_command(c, &active, &rows, &cols, prog)) {
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
    mux_raw_off();
    return 0;
}