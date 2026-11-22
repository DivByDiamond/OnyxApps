/*
 * term.h - shared terminal helpers for OnyxApps.
 *
 * Header-only so any app can include it without changing its build rule:
 *   #include "../../lib/term/term.h"
 *
 * Provides raw-mode enter/exit (saving/restoring the original settings) and
 * a TIOCGWINSZ size query, which were previously copy-pasted into vim,
 * osnake, otop, obrowse, mux, ochat and ochat-gui.
 *
 * Two raw-mode variants: term_raw_enable keeps cfmakeraw's blocking read
 * (VMIN=1), term_raw_enable_nb makes read() return immediately (VMIN=0)
 * for frame loops that poll input.
 */
#ifndef ONYX_LIB_TERM_H
#define ONYX_LIB_TERM_H

#include <termios.h>
#include <unistd.h>

static inline int term_raw_enable(struct termios *saved) {
    struct termios t;
    if (tcgetattr(0, saved) != 0) return 0;
    t = *saved;
    cfmakeraw_apply(&t);
    if (tcsetattr(0, 0, &t) != 0) return 0;
    return 1;
}

static inline int term_raw_enable_nb(struct termios *saved) {
    struct termios t;
    if (tcgetattr(0, saved) != 0) return 0;
    t = *saved;
    cfmakeraw_apply(&t);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(0, 0, &t) != 0) return 0;
    return 1;
}

static inline void term_raw_restore(const struct termios *saved) {
    tcsetattr(0, 0, (struct termios *)saved);
}

static inline int term_get_size(int *rows, int *cols) {
    unsigned short ws[4] = {24, 80, 0, 0};
    if (_onyx_ioctl(0, 0x5413, (long)ws) == 0 && ws[0] > 2 && ws[1] > 8) {
        if (rows) *rows = ws[0];
        if (cols) *cols = ws[1];
        return 1;
    }
    return 0;
}

#endif /* ONYX_LIB_TERM_H */
