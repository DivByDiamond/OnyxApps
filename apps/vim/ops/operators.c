/* operators.c — operator commands: d/c/y variants, indent. */
#include "vim.h"

/* ── Deleting with operators ───────────────────────────────────────── */
void op_delete_line(void) {
    int n = count > 0 ? count : 1;
    int i;
    yank_line_range(cy, cy + n - 1);
    for (i = 0; i < n; i++) {
        if (nlines <= 1) {
            llen[0] = 0;
            lines[0][0] = 0;
            dirty = 1;
            break;
        }
        del_line(cy);
    }
    clamp_cursor();
}

void op_yank_line(void) {
    int n = count > 0 ? count : 1;
    yank_line_range(cy, cy + n - 1);
    cx = 0;
}

void op_delete_word(void) {
    int save_cy = cy, save_cx = cx;
    motion_word_forward();
    /* delete from save to new cursor */
    {
        int from = save_cx, to = cx;
        int len, i;
        if (cy != save_cy) {
            /* spans lines: join-delete to end of this line + next word */
            to = llen[save_cy];
            cy = save_cy;
            cx = save_cx;
        }
        len = to - from;
        if (len > 0) {
            memcpy(reg_line, lines[cy] + from, len);
            reg_line[len] = 0;
            reg_line_len = len;
            reg_linewise = 0;
            for (i = from; i + len < llen[cy]; i++)
                lines[cy][i] = lines[cy][i + len];
            llen[cy] -= len;
            lines[cy][llen[cy]] = 0;
            dirty = 1;
        }
        cx = from;
    }
}

void op_delete_to_eol(void) {
    int len = llen[cy] - cx;
    if (len > 0) {
        memcpy(reg_line, lines[cy] + cx, len);
        reg_line[len] = 0;
        reg_line_len = len;
        reg_linewise = 0;
        llen[cy] = cx;
        lines[cy][cx] = 0;
        dirty = 1;
    }
}

void op_delete_to_bol(void) {
    int i;
    if (cx > 0) {
        for (i = 0; i + cx < llen[cy]; i++)
            lines[cy][i] = lines[cy][i + cx];
        llen[cy] -= cx;
        lines[cy][llen[cy]] = 0;
        cx = 0;
        dirty = 1;
    }
}

/* indent/outdent one line */
void indent_line(int dir) {
    int i;
    ensure_buffer();
    if (dir > 0) {
        if (llen[cy] + TAB_STOP >= MAX_LINE) return;
        for (i = llen[cy]; i >= 0; i--)
            lines[cy][i + TAB_STOP] = lines[cy][i];
        for (i = 0; i < TAB_STOP; i++) lines[cy][i] = ' ';
        llen[cy] += TAB_STOP;
        dirty = 1;
    } else {
        int n = 0;
        while (n < TAB_STOP && n < llen[cy] && lines[cy][n] == ' ') n++;
        if (n > 0) {
            for (i = 0; i + n < llen[cy]; i++)
                lines[cy][i] = lines[cy][i + n];
            llen[cy] -= n;
            lines[cy][llen[cy]] = 0;
            dirty = 1;
        }
    }
}
