/*
 * render.c - ANSI board rendering for osnake.
 *
 * ASCII-only glyphs (font-safe on the kernel framebuffer terminal):
 * '@' head, '#' body, '*' food, '+'/'-'/'|' border. The whole frame is
 * redrawn every tick; the grid is small (32x16) so full repaints are
 * cheap and the code stays simple.
 */
#include "osnake.h"

/* Board origin: the normal frame reserves a title, status and footer; the
 * compact frame (small consoles such as the 32x10 tablet) puts the status in
 * row 1 and uses the remaining rows for the board. */
#define R0 3
#define C0 2
#define CR0 2
#define CC0 1

static void put(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

static int board_left(void) {
    return compact_layout ? CC0 : C0;
}

static int board_top(void) {
    return compact_layout ? CR0 : R0;
}

void render_frame(struct snake *s, int paused) {
    int x, y, i;
    int top = board_top(), left = board_left();
    char rowbuf[GRID_W + 1];

    printf("\x1b[2J");
    put(1, 1);
    if (compact_layout) {
        printf("\x1b[7m osnake  score:%d  len:%d/%d \x1b[0m", s->score, s->len, MAX_LEN);
    } else {
        printf("\x1b[7m osnake - snake for OnyxOS \x1b[0m");
    }

    /* Border. */
    put(top, left);
    printf("+");
    for (x = 0; x < board_w; x++) putchar('-');
    printf("+");
    for (y = 0; y < board_h; y++) {
        put(top + 1 + y, left);
        printf("|");
        /* Build the row from the snake body, head overrides older cells. */
        memset(rowbuf, ' ', board_w);
        rowbuf[board_w] = 0;
        for (i = s->len - 1; i >= 0; i--) {
            if (s->body[i].y == y && s->body[i].x >= 0 && s->body[i].x < board_w) {
                rowbuf[s->body[i].x] = (i == 0) ? '@' : '#';
            }
        }
        printf("\x1b[32m%s\x1b[0m", rowbuf);
        printf("|");
    }
    put(top + 1 + board_h, left);
    printf("+");
    for (x = 0; x < board_w; x++) putchar('-');
    printf("+");

    /* Food (drawn after the body so it wins the cell). */
    if (s->food.x >= 0 && s->food.x < board_w && s->food.y >= 0 && s->food.y < board_h) {
        put(top + 1 + s->food.y, left + 1 + s->food.x);
        printf("\x1b[31m*\x1b[0m");
    }

    if (!compact_layout) {
        /* Status line. */
        put(top + board_h + 2, left);
        printf("\x1b[33mscore: %d   length: %d/%d\x1b[0m", s->score, s->len, MAX_LEN);

        /* Footer. */
        put(top + board_h + 3, left);
        if (paused) {
            printf("\x1b[7m PAUSED \x1b[0m arrows/wasd: steer  space: resume  q: quit");
        } else {
            printf("arrows/wasd: steer  space: pause  q: quit");
        }
    }
    fflush(stdout);
}

void render_over(struct snake *s, int won) {
    int mid_r, mid_c;
    if (compact_layout) {
        put(2, 1);
        if (won) {
            printf("\x1b[7;31m YOU WIN - board is full!  \x1b[0m");
        } else {
            printf("\x1b[7;31m         GAME OVER          \x1b[0m");
        }
        put(3, 1);
        printf("\x1b[7;31m score: %-19d \x1b[0m", s->score);
        put(4, 1);
        printf("\x1b[7;31m   press any key to exit     \x1b[0m");
        fflush(stdout);
        return;
    }

    mid_r = R0 + board_h / 2;
    mid_c = C0 + 4;

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
