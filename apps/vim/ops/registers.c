/* registers.c — yank/paste registers (linewise and charwise). */
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
