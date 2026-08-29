/* keys.c - NORMAL-mode dispatcher: pending-mode glue, counts and
 * delegation to motion/edit handlers.
 * NOTE: the trailing ZZ/ZQ block below is unreachable in the original
 * single-file version (case 'Z' returns first, and the g_pending block
 * above clears the flag for any other key). Preserved verbatim; the
 * fix (ZQ works only when reached) is tracked in the README roadmap. */
#include "vim.h"

void normal_key(int k) {
    int n = count > 0 ? count : 1;
    int r;

    if (replace_pending) {
        replace_pending = 0;
        if (k >= 0x20 && k < 0x7f && cy < nlines && cx < llen[cy]) {
            lines[cy][cx] = (char)k;
            dirty = 1;
        }
        count = 0;
        return;
    }

    /* g-pending (operator gg / plain gg) */
    if (g_pending) {
        if (k == 'g') {
            int op = g_pending;
            g_pending = 0;
            if (op == 'd') {
                yank_line_range(0, nlines - 1);
                while (nlines > 1) del_line(0);
                llen[0] = 0; lines[0][0] = 0;
            } else if (op == 'y') {
                yank_line_range(0, nlines - 1);
            } else if (op == 'c') {
                while (nlines > 1) del_line(0);
                llen[0] = 0; lines[0][0] = 0;
                mode = M_INSERT;
            } else {
                cy = 0; cx = 0;
            }
            count = 0;
            return;
        }
        g_pending = 0;
    }

    /* counts */
    if (k >= '1' && k <= '9') {
        count = count * 10 + (k - '0');
        return;
    }
    if (k == '0' && count > 0) {
        count = count * 10;
        return;
    }

    r = key_motion(k, n);
    if (r == 0) r = key_edit(k, n);
    if (r == 2) return;

    /* ZZ/ZQ second char (see NOTE above) */
    if (g_pending == 'Z') {
        g_pending = 0;
        if (k == 'Z') {
            if (fname[0] && save_to(fname) == 0) quit_flag = 1;
            else set_msg("E32: No file name");
        } else if (k == 'Q') {
            quit_flag = 1;
        }
    }

    count = 0;
}
