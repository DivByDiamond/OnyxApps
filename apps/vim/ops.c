/* ops.c — yank/paste registers and operator commands (d/c/y/>/<). */
#include "vim.h"
/* ── Registers (yank/paste) ────────────────────────────────────────── */
void yank_line_range(int from, int to) {
    int i, n;
    n = to - from + 1;
    if (n > MAX_LINES) n = MAX_LINES;
    reg_block_n = 0;
    for (i = 0; i < n; i++) {
        int src = from + i;
        if (src >= nlines) break;
        memcpy(reg_block[reg_block_n], lines[src], MAX_LINE);
        reg_block_len[reg_block_n] = llen[src];
        reg_block_n++;
    }
    reg_linewise = 1;
}

void yank_chars(int len) {
    int n = len;
    if (cx + n > llen[cy]) n = llen[cy] - cx;
    if (n < 0) n = 0;
    memcpy(reg_line, lines[cy] + cx, n);
    reg_line[n] = 0;
    reg_line_len = n;
    reg_linewise = 0;
}

void paste(int after) {
    int i, at;
    ensure_buffer();
    if (reg_linewise && reg_block_n > 0) {
        at = after ? cy + 1 : cy;
        if (nlines + reg_block_n >= MAX_LINES) return;
        for (i = nlines + reg_block_n - 1; i >= at + reg_block_n; i--) {
            memcpy(lines[i], lines[i - reg_block_n], MAX_LINE);
            llen[i] = llen[i - reg_block_n];
        }
        for (i = 0; i < reg_block_n; i++) {
            memcpy(lines[at + i], reg_block[i], MAX_LINE);
            llen[at + i] = reg_block_len[i];
        }
        nlines += reg_block_n;
        cy = at;
        cx = 0;
        dirty = 1;
    } else if (reg_line_len > 0) {
        int len = llen[cy];
        int pos = after ? cx + 1 : cx;
        int i;
        if (len + reg_line_len >= MAX_LINE) return;
        /* shift tail right, then copy in the register */
        for (i = llen[cy]; i >= pos; i--) {
            if (i + reg_line_len < MAX_LINE)
                lines[cy][i + reg_line_len] = lines[cy][i];
        }
        for (i = 0; i < reg_line_len; i++)
            lines[cy][pos + i] = reg_line[i];
        llen[cy] += reg_line_len;
        lines[cy][llen[cy]] = 0;
        cx = pos + reg_line_len - 1;
        if (cx < 0) cx = 0;
        dirty = 1;
    }
}

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
