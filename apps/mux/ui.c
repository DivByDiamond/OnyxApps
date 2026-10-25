/*
 * ui.c - status bar and redraw helpers for mux.
 *
 * The terminal is shared: child output owns lines 1..rows-1 and the last
 * line is the multiplexer's status bar. Nothing here models the child's
 * screen (there is no terminal emulator in this app); switching sessions
 * replays the session's recent raw output, which is enough for shells and
 * line-oriented tools.
 *
 * The geometry and active index are parameters, not globals: onyxcc has
 * had codegen bugs around static state, and passing two ints is free.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "mux.h"

void mux_get_size(int *rows, int *cols) {
    unsigned short ws[4];
    ws[0] = 24;
    ws[1] = 80;
    ws[2] = 0;
    ws[3] = 0;
    int r = 24, c = 80;
    if (_onyx_ioctl(0, TIOCGWINSZ, (long)ws) == 0) {
        if (ws[0] >= 4 && ws[1] >= 20) {
            r = ws[0];
            c = ws[1];
        }
    }
    *rows = r;
    *cols = c;
}

/* Move to the status line at `rows`, invert, print a bounded string. */
static void status_line(int rows, int cols, const char *text, size_t len) {
    char head[24];
    int h = 0;
    const char *pos = "\x1b[";
    while (*pos) head[h++] = *pos++;
    char digits[8];
    int d = 0;
    int v = rows > 0 ? rows : 1;
    while (v > 0 && d < 7) {
        digits[d++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (d > 0) head[h++] = digits[--d];
    head[h++] = ';';
    head[h++] = '1';
    head[h++] = 'H';
    _onyx_write(1, head, (size_t)h);
    _onyx_write(1, "\x1b[7m", 4);
    if (len > (size_t)cols) len = (size_t)cols;
    _onyx_write(1, text, len);
    _onyx_write(1, "\x1b[0m", 4);
}

void mux_status_bar(int rows, int cols, int active) {
    char line[256];
    int n = 0;
    for (int i = 0; i < MUX_MAX_SESSIONS; i++) {
        if (!g_sessions[i].used) continue;
        line[n++] = (i == active) ? '[' : ' ';
        line[n++] = (char)('1' + i);
        line[n++] = ':';
        const char *t = g_sessions[i].title;
        while (*t && n < 40) line[n++] = *t++;
        if (!g_sessions[i].alive) line[n++] = '!';
        else if (i == active) line[n++] = '*';
        line[n++] = (i == active) ? ']' : ' ';
        line[n++] = ' ';
    }
    const char *hint = "^a c:new n:next p:prev 0-9:sel r:redraw q:quit";
    if (n < 60) {
        while (*hint && n < (int)sizeof(line) - 1) line[n++] = *hint++;
    }
    status_line(rows, cols, line, (size_t)n);
}

void mux_help_line(void) {
    const char *msg =
        "\r\nmux keys: ^a c new  ^a n/p next/prev  ^a 0-9 select  "
        "^a r redraw  ^a q quit\r\n";
    _onyx_write(1, msg, strlen(msg));
}

void mux_redraw_active(int active, int rows, int cols) {
    mux_session_replay(active);
    mux_status_bar(rows, cols, active);
}
