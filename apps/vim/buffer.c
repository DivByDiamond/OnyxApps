/* buffer.c — undo recording and text editing primitives. */
#include "vim.h"
/* ── Undo ──────────────────────────────────────────────────────────── */
void undo_record_k(int line, int col, const char *deleted,
                          int dlen, int lines_removed, int kind);
void undo_record(int line, int col, const char *deleted,
                        int dlen, int lines_removed) {
    undo_record_k(line, col, deleted, dlen, lines_removed,
                  lines_removed ? UNDO_DEL_LINE : UNDO_DEL_TEXT);
}

void undo_record_k(int line, int col, const char *deleted,
                          int dlen, int lines_removed, int kind) {
    undo_t *u;
    if (nundos >= UNDO_MAX) {
        int i;
        for (i = 0; i < UNDO_MAX - 1; i++) undos[i] = undos[i + 1];
        nundos = UNDO_MAX - 1;
    }
    u = &undos[nundos++];
    u->pos_line = line;
    u->pos_col = col;
    u->deleted_len = dlen < MAX_LINE ? dlen : MAX_LINE - 1;
    if (dlen > 0 && deleted) memcpy(u->deleted, deleted, u->deleted_len);
    u->deleted[u->deleted_len] = 0;
    u->lines_removed = lines_removed;
    u->kind = kind;
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
    if (nlines >= MAX_LINES) return;
    if (cx > llen[cy]) cx = llen[cy];
    for (i = nlines; i > cy + 1; i--) {
        memcpy(lines[i], lines[i - 1], MAX_LINE);
        llen[i] = llen[i - 1];
    }
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
    for (i = row; i < nlines - 1; i++) {
        memcpy(lines[i], lines[i + 1], MAX_LINE);
        llen[i] = llen[i + 1];
    }
    nlines--;
    dirty = 1;
}
