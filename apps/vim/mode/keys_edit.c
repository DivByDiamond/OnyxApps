/* keys_edit.c - NORMAL-mode editing keys: delete/change/yank/paste,
 * replace, join, indent, undo/redo, insert/visual entry, search entry,
 * bracket match. Returns: 0 = not consumed, 1 = consumed (continue),
 * 2 = consumed (return immediately). */
#include "vim.h"

int key_edit(int k, int n) {
    switch (k) {
        /* editing */
        case 'x': case K_DEL: {
            int i;
            for (i = 0; i < n; i++) del_char_at(cy, cx);
            clamp_cursor();
            count = 0; break;
        }
        case 'X':
            if (cx > 0) {
                int i;
                cx--;
                for (i = 0; i < n; i++) del_char_at(cy, cx);
            }
            count = 0; break;
        case 'd':
            pending_op = 'd';  return 2; /* wait for second key */
        case 'y':
            pending_op = 'y';  return 2;
        case 'c':
            pending_op = 'c';  return 2;
        case 'D':
            op_delete_to_eol(); break;
        case 'C':
            op_delete_to_eol(); mode = M_INSERT; break;
        case 'S':
            undo_record(cy, 0, lines[cy], llen[cy], 0);
            llen[cy] = 0; lines[cy][0] = 0; cx = 0;
            dirty = 1; mode = M_INSERT; break;
        case 's': {
            int i;
            for (i = 0; i < n; i++) del_char_at(cy, cx);
            mode = M_INSERT; count = 0; break;
        }
        case 'p': paste(1); break;
        case 'P': paste(0); break;
        case 'r':
            replace_pending = 1;  return 2;
        case 'J':
            if (cy < nlines - 1) {
                int cl = llen[cy], nl = llen[cy + 1];
                if (cl + nl + 1 < MAX_LINE) {
                    if (cl > 0 && lines[cy][cl - 1] != ' ')
                        lines[cy][cl++] = ' ';
                    memcpy(lines[cy] + cl, lines[cy + 1], nl);
                    llen[cy] = cl + nl;
                    lines[cy][llen[cy]] = 0;
                    del_line(cy + 1);
                }
            }
            break;
        case '~':
            if (cx < llen[cy]) {
                int c = lines[cy][cx];
                if (c >= 'a' && c <= 'z') lines[cy][cx] = (char)(c - 32);
                else if (c >= 'A' && c <= 'Z') lines[cy][cx] = (char)(c + 32);
                cx++;
                dirty = 1;
            }
            break;
        case '>': indent_line(1); break;
        case '<': indent_line(-1); break;

        /* undo/redo */
        case 'u':
            do_undo();
            break;
        case K_CTRL_R:
            do_redo();
            break;

        /* insert modes */
        case 'i': mode = M_INSERT; break;
        case 'a':
            if (cx < llen[cy]) cx++;
            mode = M_INSERT; break;
        case 'I':
            cx = 0;
            while (cx < llen[cy] && lines[cy][cx] == ' ') cx++;
            mode = M_INSERT; break;
        case 'A':
            cx = llen[cy]; mode = M_INSERT; break;
        case 'o':
            /* open empty line below current */
            if (nlines < MAX_LINES) {
                int i2;
                for (i2 = nlines; i2 > cy + 1; i2--) {
                    memcpy(lines[i2], lines[i2 - 1], MAX_LINE);
                    llen[i2] = llen[i2 - 1];
                }
                llen[cy + 1] = 0;
                lines[cy + 1][0] = 0;
                nlines++;
                cy++;
                cx = 0;
                dirty = 1;
            }
            mode = M_INSERT; break;
        case 'O':
            if (nlines < MAX_LINES) {
                int i;
                for (i = nlines; i > cy; i--) {
                    memcpy(lines[i], lines[i - 1], MAX_LINE);
                    llen[i] = llen[i - 1];
                }
                llen[cy] = 0; lines[cy][0] = 0;
                nlines++;
                cx = 0;
                dirty = 1;
            }
            mode = M_INSERT; break;

        /* visual */
        case 'v':
            mode = M_VISUAL; vsy = cy; vsx = cx; break;
        case 'V':
            mode = M_VISUALL; vsy = cy; vsx = 0; break;

        /* search */
        case '/':
            mode = M_SEARCH; cmdlen = 0; cmdbuf[0] = 0; break;
        case '?':
            mode = M_SEARCH_B; cmdlen = 0; cmdbuf[0] = 0; break;
        case 'n': {
            int ly, lx;
            if (have_search && find_str(last_search, cy, cx + 1, &ly, &lx)) {
                cy = ly; cx = lx;
                set_msg("/");
            } else set_msg("E486: Pattern not found");
            break;
        }
        case 'N': {
            set_msg("(reverse search: use ?)");
            break;
        }
        case '*': {
            /* search word under cursor forward */
            int s = cx, e = cx, y, x2;
            while (s > 0 && is_word(lines[cy][s - 1])) s--;
            while (e < llen[cy] && is_word(lines[cy][e])) e++;
            if (e > s && e - s < 120) {
                memcpy(last_search, lines[cy] + s, e - s);
                last_search[e - s] = 0;
                have_search = 1;
                if (find_str(last_search, cy, e, &y, &x2)) {
                    cy = y; cx = x2;
                } else set_msg("E486: Pattern not found");
            }
            break;
        }

        /* bracket match */
        case '%':
            bracket_match();
            break;

        /* ZZ/ZQ */
        case 'Z':
            g_pending = 'Z';  return 2;

        case '.':
            set_msg("(repeat not supported yet)");
            break;

        case ':':
            mode = M_COMMAND; cmdlen = 0; cmdbuf[0] = 0; break;

        default:
            break;
    }
    return 0;
}
