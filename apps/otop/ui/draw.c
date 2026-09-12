/*
 * draw.c - drawing primitives for otop: boxes, load bars, header,
 * footer. ASCII-only glyphs (font-safe on the framebuffer terminal),
 * SGR 30-37 colors, cursor positioning via CSI H.
 */
#include "otop.h"

/* Box border in plain ASCII with an optional highlighted title. */
void draw_box(int r1, int c1, int r2, int c2, const char *title) {
    int r, c;
    if (r1 < 1 || r2 > screen_rows || c1 < 1 || c2 > screen_cols) return;
    obuf_printf("\x1b[%d;%dH+", r1, c1);
    for (c = c1 + 1; c < c2; c++) obuf_printf("-");
    obuf_printf("+");
    for (r = r1 + 1; r < r2; r++) {
        obuf_printf("\x1b[%d;%dH|", r, c1);
        obuf_printf("\x1b[%d;%dH|", r, c2);
    }
    obuf_printf("\x1b[%d;%dH+", r2, c1);
    for (c = c1 + 1; c < c2; c++) obuf_printf("-");
    obuf_printf("+");
    if (title && title[0]) {
        obuf_printf("\x1b[%d;%dH\x1b[1;36m %s \x1b[0m", r1, c1 + 2, title);
    }
}

/* Labeled load bar: green < 50%, yellow < 80%, red above. */
void draw_bar(int row, const char *label, int permille_val, const char *suffix) {
    int filled;
    int i;
    int bar_w;
    const char *col;
    if (row < 1 || row > screen_rows) return;
    bar_w = screen_cols - 14 - (int)strlen(suffix) - 2;
    if (bar_w < 10) bar_w = 10;
    if (bar_w > OTOP_BAR_W) bar_w = OTOP_BAR_W;
    obuf_printf("\x1b[%d;3H%s", row, label);
    filled = (permille_val * bar_w) / 1000;
    if (filled > bar_w) filled = bar_w;
    if (filled < 0) filled = 0;
    col = permille_val < 500 ? "\x1b[32m"
        : permille_val < 800 ? "\x1b[33m" : "\x1b[31m";
    obuf_printf("\x1b[%d;13H[", row);
    obuf_printf("%s", col);
    for (i = 0; i < bar_w; i++) obuf_printf("%c", i < filled ? '#' : ' ');
    obuf_printf("\x1b[0m] %s", suffix);
}

/* Reverse-video title line with host/release info and a spinner. */
void draw_header(struct otop_info *oi, int frame) {
    char spin[5] = "|/-\\";
    const char *host = oi->un.nodename[0] ? oi->un.nodename : "onyx";
    const char *rel = oi->un.release[0] ? oi->un.release : "dev";
    obuf_printf("\x1b[1;1H\x1b[7m otop - %s %s - OnyxOS monitor %c \x1b[0m\x1b[K",
                host, rel, spin[frame & 3]);
}

/* Reverse-video footer line (screen_rows - 1 by convention). */
void draw_footer(int row, const char *text) {
    if (row < 1 || row > screen_rows) return;
    obuf_printf("\x1b[%d;1H\x1b[7m %s \x1b[0m\x1b[K", row, text);
}
