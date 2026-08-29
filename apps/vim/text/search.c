/* search.c - pattern search, f/F/t/T char find and its pending state. */
#include "vim.h"

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

/* Consume a key while a f/F/t/T find is pending (';' ',' repeat). */
void find_pending_key(int k) {
    int dir = (find_mode == 'f' || find_mode == 't') ? 1 : -1;
    int til = (find_mode == 't' || find_mode == 'T');
    if (k == ';') {
        do_find_char(dir, til, last_find_char);
    } else if (k == ',') {
        do_find_char(-dir, til, last_find_char);
    } else if (k >= 0x20 && k < 0x7f) {
        last_find_mode = find_mode;
        last_find_char = k;
        do_find_char(dir, til, k);
    }
    find_mode = 0;
    count = 0;
}
