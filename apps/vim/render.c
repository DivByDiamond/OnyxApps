/* render.c — screen drawing, gutter, cursor placement. */
#include "vim.h"
/* ── Rendering ─────────────────────────────────────────────────────── */
int gut_width(void) {
    int w = 1, n = nlines;
    while (n >= 10) { n /= 10; w++; }
    return show_num ? w + 2 : 1;
}

void draw(void) {
    int vis, i, gw;
    query_size();
    printf("\x1b[2J");
    gw = gut_width();
    vis = rows - 2;
    if (vis < 1) vis = 1;

    for (i = 0; i < vis; i++) {
        int row = top + i;
        printf("\x1b[%d;1H", i + 1);
        if (row < nlines) {
            int maxch = cols - gw - 1;
            int len;
            if (show_num) printf("\x1b[90m%*d \x1b[0m", gw - 1, row + 1);
            else printf(" ");
            if (maxch < 1) maxch = 1;
            len = llen[row];
            if (len > maxch) len = maxch;
            /* highlight visual selection */
            if ((mode == M_VISUAL || mode == M_VISUALL)) {
                int sel_from = vsy < cy || (vsy == cy && vsx < cx) ? 0 : 0;
                (void)sel_from;
                /* simple: draw whole selected lines reversed */
                if (mode == M_VISUALL && row >= vsy && row <= cy) {
                    printf("\x1b[7m");
                    if (len > 0) fwrite(lines[row], 1, len, stdout);
                    printf("\x1b[0m");
                } else {
                    if (len > 0) fwrite(lines[row], 1, len, stdout);
                }
            } else {
                if (len > 0) fwrite(lines[row], 1, len, stdout);
            }
        } else if (row == nlines) {
            printf("\x1b[90m~\x1b[0m");
        }
    }

    /* status line */
    printf("\x1b[%d;1H\x1b[7m", rows - 1);
    {
        char m[24];
        if (mode == M_NORMAL) strcpy(m, "NORMAL");
        else if (mode == M_INSERT) strcpy(m, "INSERT");
        else if (mode == M_COMMAND) strcpy(m, "COMMAND");
        else if (mode == M_SEARCH || mode == M_SEARCH_B) strcpy(m, "SEARCH");
        else if (mode == M_VISUAL) strcpy(m, "VISUAL");
        else if (mode == M_VISUALL) strcpy(m, "V-LINE");
        else strcpy(m, "?");
        printf(" [%s] %s%s  L%d/%d C%d ", m,
               fname[0] ? fname : "[No Name]",
               dirty ? "+" : "  ", cy + 1, nlines, cx + 1);
    }
    printf("\x1b[0m\x1b[K");

    /* command/message line */
    printf("\x1b[%d;1H", rows);
    if (mode == M_COMMAND) {
        printf(":%s", cmdbuf);
    } else if (mode == M_SEARCH) {
        printf("/%s", cmdbuf);
    } else if (mode == M_SEARCH_B) {
        printf("?%s", cmdbuf);
    } else {
        printf("%s", msg);
    }
    printf("\x1b[K");
    fflush(stdout);
}

void place_cursor(void) {
    int gw = gut_width();
    int srow = cy - top + 1;
    int scol = cx + 1 + gw;
    if (srow < 1) srow = 1;
    if (scol < 1) scol = 1;
    printf("\x1b[%d;%dH", srow, scol);
    fflush(stdout);
}

void set_msg(const char *m) {
    strncpy(msg, m, sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = 0;
}
