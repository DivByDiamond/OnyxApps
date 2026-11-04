/* draw.c - paint the 240x112 logical screen into an RGB565 buffer. */
#include "gui.h"
#include "graphics/raster.h"
#include <string.h>

#define BG 0x0821
#define PANE_X 74
#define PANE_Y 11
#define LINE_H 9

static void fill(unsigned short *px, int x, int y, int w, int h, unsigned short c) {
    int a, b;
    for (b = y; b < y + h && b < GUI_H; b++)
        for (a = x; a < x + w && a < GUI_W; a++)
            if (a >= 0 && b >= 0) px[b * GUI_W + a] = c;
}

/* Draw at most `max` chars of s; 6 px advance, 8 rows. Clipped to the screen. */
static void text(unsigned short *px, int x, int y, const char *s, int max, unsigned short c) {
    int i, row, col;
    for (i = 0; s[i] && i < max; i++)
        for (row = 0; row < 8; row++) {
            unsigned int bits = gui_glyph8((unsigned char)s[i], row);
            for (col = 0; col < 5; col++) {
                int X = x + i * 6 + col, Y = y + row;
                if ((bits & (1u << (4 - col))) && X >= 0 && Y >= 0 && X < GUI_W && Y < GUI_H)
                    px[Y * GUI_W + X] = c;
            }
        }
}

static void pane(const struct gui *g, unsigned short *px) {
    static char rows[GUI_PANE_LINES][GUI_PANE_COLS + 1];
    static char folded[OCHAT_MAX_TEXT + 4];
    static int st[128], ln[128];
    char mine[GUI_PANE_LINES];
    const char *peer = gui_selected(g);
    int free_rows = g->status[0] ? GUI_PANE_LINES - 1 : GUI_PANE_LINES;
    int slot = free_rows, i, n, k;
    fill(px, PANE_X, PANE_Y, GUI_W - PANE_X, GUI_PANE_LINES * LINE_H, 0);
    for (i = ochat_hist_count(&g->hist) - 1; peer && i >= 0 && slot > 0; i--) {
        const struct ochat_entry *e = ochat_hist_get(&g->hist, i);
        if (strcmp(e->peer, peer)) continue;
        folded[0] = e->outgoing ? '>' : '<';
        folded[1] = ' ';
        ochat_ascii_fold(e->text, folded + 2, sizeof(folded) - 2);
        n = ochat_wrap(folded, GUI_PANE_COLS, st, ln, 128);
        for (k = n - 1; k >= 0 && slot > 0; k--) {
            slot--;
            memcpy(rows[slot], folded + st[k], (size_t)ln[k]);
            rows[slot][ln[k]] = 0;
            mine[slot] = (char)e->outgoing;
        }
    }
    for (i = slot; i < free_rows; i++)
        text(px, PANE_X + 1, PANE_Y + i * LINE_H, rows[i], GUI_PANE_COLS, mine[i] ? 0x07E0 : 0xFFFF);
    if (!peer) text(px, PANE_X + 1, PANE_Y, "SELECT A CONTACT", GUI_PANE_COLS, 0x8410);
    if (g->status[0]) {
        fill(px, PANE_X, PANE_Y + free_rows * LINE_H, GUI_W - PANE_X, LINE_H, 0x4000);
        text(px, PANE_X + 1, PANE_Y + free_rows * LINE_H, g->status, GUI_PANE_COLS, 0xFD20);
    }
}

static void input(const struct gui *g, unsigned short *px) {
    char folded[OCHAT_LINE_MAX * 2 + 2];
    int len, show = 31, start;
    fill(px, 0, 102, 200, 10, 0x0000);
    text(px, 2, 103, ">", 1, 0x07FF);
    ochat_ascii_fold(g->line.buf, folded, sizeof(folded));
    len = (int)strlen(folded);
    start = len > show - 1 ? len - (show - 1) : 0;     /* show the tail while typing */
    text(px, 10, 103, folded + start, show, 0xFFFF);
    fill(px, 10 + (len - start) * 6, 110, 5, 1, 0xFFFF);  /* cursor underscore */
}

void gui_draw(const struct gui *g, unsigned short *px) {
    static const unsigned short dot[] = { 0x8410, 0xFFE0, 0x07E0, 0xF800, 0xF800 };
    struct ow_raster r;
    fill(px, 0, 0, GUI_W, GUI_H, BG);
    if (ow_raster_init(&r, px, GUI_W, GUI_H, GUI_W, gui_glyph)) ow_raster_paint(&g->view, &r);
    pane(g, px);
    input(g, px);
    fill(px, 134, 2, 8, 7, dot[g->ping_state]);
}
