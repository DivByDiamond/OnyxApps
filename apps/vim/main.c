/* main.c — program entry: argument handling and the editor loop. */
#include "vim.h"

/* ── Main ──────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    int k;

    if (argc > 1) {
        load_file(argv[1]);
    } else {
        nlines = 1;
        llen[0] = 0;
        lines[0][0] = 0;
        strcpy(msg, "vim 1.0 — :h for help");
    }

    query_size();
    raw_enable();

    while (!quit_flag) {
        clamp_cursor();
        scroll_view();
        draw();
        place_cursor();
        k = read_key();
        if (k == 0) continue;

        if (mode == M_NORMAL) {
            normal_key(k);
        } else if (mode == M_INSERT) {
            if (k == 0x1b) {
                if (cx > 0) cx--;         /* vim moves back one on Esc */
                mode = M_NORMAL;
            } else if (k == '\r' || k == '\n') {
                split_line();
            } else if (k == 0x7f || k == 0x08) {
                if (cx > 0) {
                    cx--;
                    del_char_at(cy, cx);
                } else if (cy > 0) {
                    /* join to previous line */
                    int pl = llen[cy - 1];
                    int tl = llen[cy];
                    if (pl + tl < MAX_LINE) {
                        memcpy(lines[cy - 1] + pl, lines[cy], tl);
                        llen[cy - 1] = pl + tl;
                        lines[cy - 1][llen[cy - 1]] = 0;
                        del_line(cy);
                        cy--;
                        cx = pl;
                    }
                }
            } else if (k == '\t') {
                int i;
                for (i = 0; i < TAB_STOP; i++) insert_char(' ');
            } else if (k >= 0x20 && k < 0x7f) {
                insert_char(k);
            } else if (k == K_LEFT) { if (cx > 0) cx--; }
            else if (k == K_RIGHT) { if (cx < llen[cy]) cx++; }
            else if (k == K_UP) { if (cy > 0) { cy--; clamp_cursor(); } }
            else if (k == K_DOWN) { if (cy < nlines - 1) { cy++; clamp_cursor(); } }
        } else if (mode == M_COMMAND || mode == M_SEARCH || mode == M_SEARCH_B) {
            if (k == 0x1b) {
                mode = M_NORMAL;
            } else if (k == '\r' || k == '\n') {
                cmdbuf[cmdlen] = 0;
                if (mode == M_COMMAND) {
                    run_command(cmdbuf);
                } else {
                    int ly, lx;
                    strncpy(last_search, cmdbuf, sizeof(last_search) - 1);
                    last_search[sizeof(last_search) - 1] = 0;
                    have_search = 1;
                    if (find_str(cmdbuf, cy, cx, &ly, &lx)) {
                        cy = ly; cx = lx;
                        set_msg("/");
                    } else {
                        set_msg("E486: Pattern not found");
                    }
                }
                mode = M_NORMAL;
            } else if (k == 0x7f || k == 0x08) {
                if (cmdlen > 0) cmdlen--;
                cmdbuf[cmdlen] = 0;
            } else if (k >= 0x20 && k < 0x7f) {
                if (cmdlen < (int)sizeof(cmdbuf) - 1) {
                    cmdbuf[cmdlen++] = (char)k;
                    cmdbuf[cmdlen] = 0;
                }
            }
        } else if (mode == M_VISUAL || mode == M_VISUALL) {
            if (k == 0x1b) {
                mode = M_NORMAL;
            } else if (k == 'd' || k == 'x') {
                int from = vsy < cy ? vsy : cy;
                int to = vsy < cy ? cy : vsy;
                yank_line_range(from, to);
                {
                    int i;
                    for (i = from; i <= to && nlines > 1; i++) del_line(from);
                    if (nlines == 1 && to - from + 1 >= 1 && from == 0) {
                        llen[0] = 0; lines[0][0] = 0;
                    }
                }
                cy = from; if (cy >= nlines) cy = nlines - 1;
                cx = 0;
                mode = M_NORMAL;
            } else if (k == 'y') {
                int from = vsy < cy ? vsy : cy;
                int to = vsy < cy ? cy : vsy;
                yank_line_range(from, to);
                cy = from; cx = 0;
                mode = M_NORMAL;
            } else if (k == '>') {
                int from = vsy < cy ? vsy : cy;
                int to = vsy < cy ? cy : vsy;
                int save = cy, i;
                for (i = from; i <= to; i++) {
                    cy = i;
                    indent_line(1);
                }
                cy = save;
            } else if (k == '<') {
                int from = vsy < cy ? vsy : cy;
                int to = vsy < cy ? cy : vsy;
                int save = cy, i;
                for (i = from; i <= to; i++) {
                    cy = i;
                    indent_line(-1);
                }
                cy = save;
            } else if (k == '~') {
                int from = vsy < cy ? vsy : cy;
                int to = vsy < cy ? cy : vsy;
                int save = cy, i;
                for (i = from; i <= to; i++) {
                    int c2;
                    cy = i; cx = 0;
                    for (c2 = 0; c2 < llen[i]; c2++) {
                        int ch = lines[i][c2];
                        if (ch >= 'a' && ch <= 'z') lines[i][c2] = (char)(ch - 32);
                        else if (ch >= 'A' && ch <= 'Z') lines[i][c2] = (char)(ch + 32);
                    }
                    dirty = 1;
                }
                cy = save;
                mode = M_NORMAL;
            } else if (k == ':') {
                mode = M_COMMAND; cmdlen = 0; cmdbuf[0] = 0;
            } else {
                /* motions extend selection */
                switch (k) {
                    case 'h': case K_LEFT: if (cx > 0) cx--; break;
                    case 'l': case K_RIGHT: if (cx < llen[cy]) cx++; break;
                    case 'j': case K_DOWN: if (cy < nlines - 1) { cy++; clamp_cursor(); } break;
                    case 'k': case K_UP: if (cy > 0) { cy--; clamp_cursor(); } break;
                    case 'w': motion_word_forward(); break;
                    case 'b': motion_word_back(); break;
                    case '0': cx = 0; break;
                    case '$': cx = llen[cy]; break;
                    case 'G': cy = nlines - 1; cx = 0; break;
                    case 'g':
                        /* gg in visual = select to top */
                        cy = 0; cx = 0; break;
                }
            }
        }
    }

    printf("\x1b[2J\x1b[1;1H\x1b[?25h");
    fflush(stdout);
    raw_disable();
    return 0;
}
