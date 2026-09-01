/*
 * render.c - ANSI board rendering for osnake.
 *
 * ASCII-only glyphs (font-safe on the kernel framebuffer terminal):
 * '@' head, '#' body, '*' food, '+'/'-'/'|' border. The whole frame is
 * redrawn every tick; the grid is small (32x16) so full repaints are
 * cheap and the code stays simple.
 */
#include "osnake.h"

/* Board origin on screen. */
#define R0 3
#define C0 2

static void put(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

void render_frame(struct snake *s, int paused) {
    int x, y, i;
    char rowbuf[GRID_W + 1];

    printf("\x1b[2J");
    put(1, 1);
    printf("\x1b[7m osnake - snake for OnyxOS \x1b[0m");

    /* Border. */
    put(R0, C0);
    printf("+");
    for (x = 0; x < GRID_W; x++) putchar('-');
    printf("+");
    for (y = 0; y < GRID_H; y++) {
        put(R0 + 1 + y, C0);
        printf("|");
        /* Build the row from the snake body, head overrides older cells. */
        memset(rowbuf, ' ', GRID_W);
        rowbuf[GRID_W] = 0;
        for (i = s->len - 1; i >= 0; i--) {
            if (s->body[i].y == y) {
                rowbuf[s->body[i].x] = (i == 0) ? '@' : '#';
            }
        }
        printf("\x1b[32m%s\x1b[0m", rowbuf);
        printf("|");
    }
    put(R0 + 1 + GRID_H, C0);
    printf("+");
    for (x = 0; x < GRID_W; x++) putchar('-');
    printf("+");

    /* Food (drawn after the body so it wins the cell). */
    if (s->food.x >= 0) {
        put(R0 + 1 + s->food.y, C0 + 1 + s->food.x);
        printf("\x1b[31m*\x1b[0m");
    }

    /* Status line. */
    put(R0 + GRID_H + 2, C0);
    printf("\x1b[33mscore: %d   length: %d/%d\x1b[0m", s->score, s->len, MAX_LEN);

    /* Footer. */
    put(R0 + GRID_H + 3, C0);
    if (paused) {
        printf("\x1b[7m PAUSED \x1b[0m arrows/wasd: steer  space: resume  q: quit");
    } else {
        printf("arrows/wasd: steer  space: pause  q: quit");
    }
    fflush(stdout);
}

void render_over(struct snake *s, int won) {
    int mid_r = R0 + GRID_H / 2;
    int mid_c = C0 + 4;

    put(mid_r, mid_c);
    printf("\x1b[7;31m +----------------------------+ \x1b[0m");
    put(mid_r + 1, mid_c);
    if (won) {
        printf("\x1b[7;31m |  YOU WIN - board is full!  | \x1b[0m");
    } else {
        printf("\x1b[7;31m |         GAME OVER          | \x1b[0m");
    }
    put(mid_r + 2, mid_c);
    printf("\x1b[7;31m |  score: %-19d | \x1b[0m", s->score);
    put(mid_r + 3, mid_c);
    printf("\x1b[7;31m |   press any key to exit    | \x1b[0m");
    put(mid_r + 4, mid_c);
    printf("\x1b[7;31m +----------------------------+ \x1b[0m");
    fflush(stdout);
}
