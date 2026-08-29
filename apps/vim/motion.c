/* motion.c — word motions and search primitives. */
#include "vim.h"
/* ── Word motions ──────────────────────────────────────────────────── */
int is_word(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

void motion_word_forward(void) {
    int started_new;
    ensure_buffer();
    if (cy >= nlines) return;
    started_new = 0;
    for (;;) {
        if (cx >= llen[cy]) {
            if (cy < nlines - 1) {
                cy++;
                cx = 0;
                started_new = 1;
                continue;
            }
            return;
        }
        if (is_word(lines[cy][cx])) {
            if (started_new) return;
            while (cx < llen[cy] && is_word(lines[cy][cx])) cx++;
            /* skip one separator */
            if (cx < llen[cy]) cx++;
            return;
        }
        if (!started_new && cx == 0 && llen[cy] > 0 && is_word(lines[cy][0])) {
            /* at sep at line start */
        }
        cx++;
    }
}

void motion_word_back(void) {
    ensure_buffer();
    for (;;) {
        if (cx == 0) {
            if (cy > 0) {
                cy--;
                cx = llen[cy];
                continue;
            }
            cx = 0;
            return;
        }
        cx--;
        if (cx < llen[cy] && is_word(lines[cy][cx])) {
            while (cx > 0 && is_word(lines[cy][cx - 1])) cx--;
            return;
        }
    }
}

void motion_word_end(void) {
    ensure_buffer();
    for (;;) {
        if (cx + 1 >= llen[cy]) {
            if (cy < nlines - 1) {
                cy++;
                cx = 0;
                continue;
            }
            return;
        }
        cx++;
        if (cx < llen[cy] && is_word(lines[cy][cx])) {
            if (cx + 1 <= llen[cy] && (cx + 1 >= llen[cy] ||
                                       !is_word(lines[cy][cx + 1]))) {
                if (cx + 1 < llen[cy] && is_word(lines[cy][cx + 1])) continue;
                return;
            }
        }
    }
}

/* ── Search ────────────────────────────────────────────────────────── */
int find_str(const char *pat, int from_line, int from_col,
                    int *out_line, int *out_col) {
    int y, pos;
    char *hit;
    if (!pat[0]) return 0;
    for (y = from_line; y < nlines; y++) {
        lines[y][llen[y]] = 0;
        hit = strstr(lines[y] + (y == from_line ? from_col : 0), pat);
        if (hit) {
            pos = (int)(hit - lines[y]);
            *out_line = y;
            *out_col = pos;
            return 1;
        }
    }
    /* wrap */
    for (y = 0; y <= from_line && y < nlines; y++) {
        lines[y][llen[y]] = 0;
        hit = strstr(lines[y], pat);
        if (hit && (y < from_line || (hit - lines[y]) < from_col)) {
            pos = (int)(hit - lines[y]);
            *out_line = y;
            *out_col = pos;
            return 1;
        }
    }
    return 0;
}

/* ── f/F/t/T char find ─────────────────────────────────────────────── */
int do_find_char(int dir, int til, int c) {
    int i;
    if (dir > 0) {
        for (i = cx + 1; i < llen[cy]; i++) {
            if (lines[cy][i] == c) {
                cx = til ? i - 1 : i;
                return 1;
            }
        }
    } else {
        for (i = cx - 1; i >= 0; i--) {
            if (lines[cy][i] == c) {
                cx = til ? i + 1 : i;
                return 1;
            }
        }
    }
    return 0;
}
