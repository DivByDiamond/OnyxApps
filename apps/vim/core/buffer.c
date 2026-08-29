/* buffer.c - text editing primitives and shared line-shift helpers. */
#include "vim.h"

/* Shift lines [at..nlines-1] down by one to open a slot at `at`.
 * Returns 0 when the buffer is full. */
int make_room(int at) {
    int i;
    if (nlines >= MAX_LINES) return 0;
    for (i = nlines; i > at; i--) {
        memcpy(lines[i], lines[i - 1], MAX_LINE);
        llen[i] = llen[i - 1];
    }
    return 1;
}

/* Close the gap at `row` by shifting the tail up. */
void remove_at(int row) {
    int i;
    for (i = row; i < nlines - 1; i++) {
        memcpy(lines[i], lines[i + 1], MAX_LINE);
        llen[i] = llen[i + 1];
    }
    nlines--;
}

/* ── Text editing primitives ───────────────────────────────────────── */
void insert_char(int c) {
    int len, i;
    char cc = (char)c;
    ensure_buffer();
    undo_record_k(cy, cx, &cc, 1, 0, UNDO_INS_TEXT);
    len = llen[cy];
    if (len >= MAX_LINE - 1) return;
    if (cx > len) cx = len;
    for (i = len; i > cx; i--) lines[cy][i] = lines[cy][i - 1];
    lines[cy][cx] = (char)c;
    llen[cy]++;
    lines[cy][llen[cy]] = 0;
    cx++;
    dirty = 1;
}

void split_line(void) {
    int i, tail;
    ensure_buffer();
    if (cx > llen[cy]) cx = llen[cy];
    if (!make_room(cy + 1)) return;
    tail = llen[cy] - cx;
    if (tail > 0) memcpy(lines[cy + 1], lines[cy] + cx, tail);
    lines[cy + 1][tail] = 0;
    llen[cy + 1] = tail;
    llen[cy] = cx;
    lines[cy][cx] = 0;
    nlines++;
    cy++;
    cx = 0;
    dirty = 1;
}

void del_char_at(int row, int col) {
    int len, i;
    if (row >= nlines) return;
    len = llen[row];
    if (col >= len) return;
    {
        char c = lines[row][col];
        undo_record(row, col, &c, 1, 0);
    }
    for (i = col; i < len; i++) lines[row][i] = lines[row][i + 1];
    llen[row]--;
    lines[row][llen[row]] = 0;
    dirty = 1;
}

void del_line(int row) {
    int i;
    if (row >= nlines || nlines <= 1) {
        if (nlines == 1) {
            undo_record(row, 0, lines[row], llen[row], 0);
            llen[row] = 0;
            lines[row][0] = 0;
            dirty = 1;
        }
        return;
    }
    undo_record_k(row, 0, lines[row], llen[row], 1, UNDO_DEL_LINE);
    remove_at(row);
    dirty = 1;
}
