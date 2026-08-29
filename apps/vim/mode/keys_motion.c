/* keys_motion.c — NORMAL-mode operator-pending and motion/scroll keys.
 * Return codes: 0 = not consumed, 1 = consumed (continue to trailing
 * logic), 2 = consumed (return immediately). */
#include "vim.h"

/* Operator-pending d/c/y/g (dd dw d$ d0 dg dj dk, same for y/c):
 * returns 2 for the operator-g path (original skips count reset). */
static int operator_key(int k, int n) {
    switch (k) {
            case 'd':
                if (pending_op == 'd') { count = n; op_delete_line(); }
                break;
            case 'y':
                if (pending_op == 'y') { count = n; op_yank_line(); }
                break;
            case 'c':
                if (pending_op == 'c') {
                    count = n;
                    op_delete_line();
                    mode = M_INSERT;
                }
                break;
            case 'w':
                if (pending_op == 'd') op_delete_word();
                else if (pending_op == 'y') {
                    int save = cx;
                    motion_word_forward();
                    yank_chars(cx - save);
                    cx = save;
                } else if (pending_op == 'c') {
                    op_delete_word();
                    mode = M_INSERT;
                }
                break;
            case '$':
                if (pending_op == 'd') op_delete_to_eol();
                else if (pending_op == 'y') {
                    int len = llen[cy] - cx;
                    memcpy(reg_line, lines[cy] + cx, len);
                    reg_line[len] = 0;
                    reg_line_len = len;
                    reg_linewise = 0;
                } else if (pending_op == 'c') {
                    op_delete_to_eol();
                    mode = M_INSERT;
                }
                break;
            case '0':
                if (pending_op == 'd') op_delete_to_bol();
                else if (pending_op == 'c') { op_delete_to_bol(); mode = M_INSERT; }
                break;
            case 'g':
                if (pending_op == 'd' || pending_op == 'y' || pending_op == 'c') {
                    g_pending = pending_op;
                    pending_op = 0;
                     return 2; /* wait for second g */
                }
                break;
            case 'j':
                if (pending_op == 'd') {
                    yank_line_range(cy, cy + 1);
                    del_line(cy);
                    if (cy < nlines) del_line(cy);
                } else if (pending_op == 'y') {
                    yank_line_range(cy, cy + 1);
                }
                break;
            case 'k':
                if (pending_op == 'd') {
                    if (cy > 0) {
                        yank_line_range(cy - 1, cy);
                        del_line(cy - 1);
                        del_line(cy - 1);
                    }
                } else if (pending_op == 'y') {
                    if (cy > 0) yank_line_range(cy - 1, cy);
                }
    }
    pending_op = 0;
    count = 0;
    return 0;
}

/* Motion, scroll and goto keys. */
static int motion_cases(int k, int n) {
    switch (k) {
        /* motions */
        case 'h': case K_LEFT:
            cx -= n; if (cx < 0) cx = 0; count = 0; break;
        case 'l': case K_RIGHT:
            cx += n; if (cx > llen[cy]) cx = llen[cy]; count = 0; break;
        case 'j': case K_DOWN:
            cy += n; clamp_cursor(); count = 0; break;
        case 'k': case K_UP:
            cy -= n; if (cy < 0) cy = 0; clamp_cursor(); count = 0; break;
        case 'w':
            { int i; for (i = 0; i < n; i++) motion_word_forward(); }
            count = 0; break;
        case 'b':
            { int i; for (i = 0; i < n; i++) motion_word_back(); }
            count = 0; break;
        case 'e':
            { int i; for (i = 0; i < n; i++) motion_word_end(); }
            count = 0; break;
        case '0': case K_HOME:
            cx = 0; count = 0; break;
        case '$': case K_END:
            cx = llen[cy]; count = 0; break;
        case '^':
            cx = 0;
            while (cx < llen[cy] && lines[cy][cx] == ' ') cx++;
            count = 0; break;
        case 'G':
            if (count > 0) { cy = count - 1; if (cy >= nlines) cy = nlines - 1; }
            else cy = nlines - 1;
            cx = 0; count = 0; break;
        case 'g':
            g_pending = 1;  return 2;
        case '{':
            { int i; for (i = 0; i < n && cy > 0; i++) {
                cy--;
                while (cy > 0 && llen[cy] != 0) cy--;
            } }
            cx = 0; count = 0; break;
        case '}':
            { int i; for (i = 0; i < n && cy < nlines - 1; i++) {
                cy++;
                while (cy < nlines - 1 && llen[cy] != 0) cy++;
            } }
            cx = 0; count = 0; break;
        case 'f': case 'F': case 't': case 'T':
            find_mode = k;  return 2;
        case ';':
            if (last_find_char) {
                int dir = (last_find_mode == 'f' || last_find_mode == 't') ? 1 : -1;
                int til = (last_find_mode == 't' || last_find_mode == 'T');
                do_find_char(dir, til, last_find_char);
            }
            break;
        case ',':
            if (last_find_char) {
                int dir = (last_find_mode == 'f' || last_find_mode == 't') ? 1 : -1;
                int til = (last_find_mode == 't' || last_find_mode == 'T');
                do_find_char(-dir, til, last_find_char);
            }
            break;

        /* scrolling */
        case K_CTRL_F: case K_PGDN:
            cy += rows - 2; clamp_cursor(); break;
        case K_CTRL_B: case K_PGUP:
            cy -= rows - 2; if (cy < 0) cy = 0; clamp_cursor(); break;
        case K_CTRL_D:
            cy += (rows - 2) / 2; clamp_cursor(); break;
        case K_CTRL_U:
            cy -= (rows - 2) / 2; if (cy < 0) cy = 0; clamp_cursor(); break;
        case 'z':
            /* zz — center */
            top = cy - (rows - 2) / 2;
            if (top < 0) top = 0;
            break;
    }
    return 0;
}

int key_motion(int k, int n) {
    if (find_mode) {
        find_pending_key(k);
        return 2;
    }
    if (pending_op) {
        int r = operator_key(k, n);
        if (r == 0) {
            pending_op = 0;
            count = 0;
        }
        return 2;
    }
    return motion_cases(k, n);
}
