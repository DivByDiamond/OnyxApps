/* keys.c — NORMAL-mode key dispatch (single dispatcher function,
 * ~525 lines; the only file above the 250-line rule — splitting the
 * mode switch is tracked in README roadmap). */
#include "vim.h"
/* ── Normal-mode key dispatch ──────────────────────────────────────── */
void normal_key(int k) {
    int n = count > 0 ? count : 1;

    /* pending f/F/t/T char */
    if (find_mode) {
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
        return;
    }

    if (replace_pending) {
        replace_pending = 0;
        if (k >= 0x20 && k < 0x7f && cy < nlines && cx < llen[cy]) {
            lines[cy][cx] = (char)k;
            dirty = 1;
        }
        count = 0;
        return;
    }

    /* pending operator d/c/y/g */
    if (pending_op) {
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
                    return; /* wait for second g */
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
                break;
        }
        if (g_pending) {
            /* handled by next key via pending path below */
        }
        pending_op = 0;
        count = 0;
        return;
    }

    /* g-pending (gg/G) */
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
            g_pending = 1; return;
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
            find_mode = k; return;
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
            pending_op = 'd'; return; /* wait for second key */
        case 'y':
            pending_op = 'y'; return;
        case 'c':
            pending_op = 'c'; return;
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
            replace_pending = 1; return;
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
        case 'u': {
            if (nundos > 0) {
                undo_t *u = &undos[--nundos];
                if (nredos < UNDO_MAX) {
                    memcpy(&redos[nredos], u, sizeof(undo_t));
                    nredos++;
                }
                if (u->kind == UNDO_DEL_LINE) {
                    int i;
                    if (nlines < MAX_LINES) {
                        for (i = nlines; i > u->pos_line; i--) {
                            memcpy(lines[i], lines[i - 1], MAX_LINE);
                            llen[i] = llen[i - 1];
                        }
                        memcpy(lines[u->pos_line], u->deleted, u->deleted_len);
                        lines[u->pos_line][u->deleted_len] = 0;
                        llen[u->pos_line] = u->deleted_len;
                        nlines++;
                    }
                } else if (u->kind == UNDO_DEL_TEXT) {
                    /* re-insert the deleted text */
                    int len = llen[u->pos_line];
                    int i;
                    for (i = len; i >= u->pos_col; i--)
                        if (i + u->deleted_len < MAX_LINE)
                            lines[u->pos_line][i + u->deleted_len] =
                                lines[u->pos_line][i];
                    for (i = 0; i < u->deleted_len; i++)
                        lines[u->pos_line][u->pos_col + i] = u->deleted[i];
                    llen[u->pos_line] += u->deleted_len;
                } else {
                    /* UNDO_INS_TEXT: delete the inserted chars */
                    int i, len = llen[u->pos_line];
                    for (i = u->pos_col; i + u->deleted_len < len; i++)
                        lines[u->pos_line][i] =
                            lines[u->pos_line][i + u->deleted_len];
                    llen[u->pos_line] -= u->deleted_len;
                    if (llen[u->pos_line] < 0) llen[u->pos_line] = 0;
                    lines[u->pos_line][llen[u->pos_line]] = 0;
                }
                cy = u->pos_line;
                cx = u->pos_col;
                set_msg("Undo");
            } else {
                set_msg("Already at oldest change");
            }
            break;
        }
        case K_CTRL_R: {
            if (nredos > 0) {
                undo_t *u = &redos[--nredos];
                if (nundos < UNDO_MAX) {
                    memcpy(&undos[nundos], u, sizeof(undo_t));
                    nundos++;
                }
                if (u->kind == UNDO_DEL_LINE) {
                    int i;
                    for (i = u->pos_line; i < nlines - 1; i++) {
                        memcpy(lines[i], lines[i + 1], MAX_LINE);
                        llen[i] = llen[i + 1];
                    }
                    nlines--;
                } else if (u->kind == UNDO_DEL_TEXT) {
                    /* re-delete the text */
                    int i, len = llen[u->pos_line], dl = u->deleted_len;
                    for (i = u->pos_col; i + dl < len; i++)
                        lines[u->pos_line][i] = lines[u->pos_line][i + dl];
                    llen[u->pos_line] -= dl;
                    if (llen[u->pos_line] < 0) llen[u->pos_line] = 0;
                    lines[u->pos_line][llen[u->pos_line]] = 0;
                } else {
                    /* UNDO_INS_TEXT: re-insert */
                    int len = llen[u->pos_line];
                    int i;
                    for (i = len; i >= u->pos_col; i--)
                        if (i + u->deleted_len < MAX_LINE)
                            lines[u->pos_line][i + u->deleted_len] =
                                lines[u->pos_line][i];
                    for (i = 0; i < u->deleted_len; i++)
                        lines[u->pos_line][u->pos_col + i] = u->deleted[i];
                    llen[u->pos_line] += u->deleted_len;
                }
                cy = u->pos_line;
                cx = u->pos_col;
                set_msg("Redo");
            } else {
                set_msg("Already at newest change");
            }
            break;
        }

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
        case '%': {
            static const char open_b[] = "([{";
            static const char close_b[] = ")]}";
            int i, depth = 0, dir = 0;
            char me = 0, mate = 0;
            for (i = 0; i < 3; i++) {
                if (cx < llen[cy] && lines[cy][cx] == open_b[i]) {
                    me = open_b[i]; mate = close_b[i]; dir = 1; break;
                }
                if (cx < llen[cy] && lines[cy][cx] == close_b[i]) {
                    me = close_b[i]; mate = open_b[i]; dir = -1; break;
                }
            }
            if (me) {
                int y2 = cy, x2 = cx;
                for (;;) {
                    x2 += dir;
                    if (x2 < 0) {
                        if (y2 == 0) break;
                        y2--; x2 = llen[y2] - 1;
                        if (x2 < 0) x2 = 0;
                    } else if (x2 >= llen[y2]) {
                        if (y2 >= nlines - 1) break;
                        y2++; x2 = 0;
                    }
                    if (x2 < llen[y2]) {
                        char c2 = lines[y2][x2];
                        if (c2 == me) depth++;
                        else if (c2 == mate) {
                            if (depth == 0) { cy = y2; cx = x2; break; }
                            depth--;
                        }
                    }
                }
            }
            break;
        }

        /* ZZ/ZQ */
        case 'Z':
            g_pending = 'Z'; return;

        case '.':
            set_msg("(repeat not supported yet)");
            break;

        case ':':
            mode = M_COMMAND; cmdlen = 0; cmdbuf[0] = 0; break;

        default:
            break;
    }

    /* ZZ/ZQ second char */
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
