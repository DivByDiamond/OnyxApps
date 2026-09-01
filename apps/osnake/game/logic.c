/*
 * logic.c - pure game state transitions for osnake, no I/O.
 *
 * The board is a GRID_W x GRID_H cell grid; walls kill, touching the
 * snake body kills, eating food grows the snake and adds 10 points.
 * All functions only touch the passed struct, so they run unchanged on
 * the host (tests/native) and under onyxcc on OnyxOS.
 */
#include "osnake.h"

/* Place the snake in the middle of the board, heading right. */
void game_reset(struct snake *s, unsigned long seed) {
    int i;
    int cy = GRID_H / 2;
    int cx = GRID_W / 2;

    memset(s, 0, sizeof(*s));
    s->rng = seed ? seed : 1;
    s->len = 3;
    s->dir = DIR_RIGHT;
    s->pending_dir = DIR_RIGHT;
    s->alive = 1;
    for (i = 0; i < s->len; i++) {
        s->body[i].x = cx - i;
        s->body[i].y = cy;
    }
    game_food_spawn(s);
}

/* Queue a turn; 180 degree reversals are ignored (dir XOR 2 = reverse). */
void game_set_dir(struct snake *s, int dir) {
    if (dir < DIR_UP || dir > DIR_LEFT) return;
    if (dir == (s->dir ^ 2)) return;
    s->pending_dir = dir;
}

/* Does (x,y) hit body[0..n-1]? skip_tail drops the vacated tail cell. */
static int hits_body(struct snake *s, int skip_tail, int x, int y) {
    int n = s->len;
    int i;

    if (skip_tail && n > 0) n--;
    for (i = 0; i < n; i++) {
        if (s->body[i].x == x && s->body[i].y == y) return 1;
    }
    return 0;
}

/* Advance the simulation by one cell. Returns 0 when the game is over. */
int game_step(struct snake *s) {
    int hx, hy, grow, newlen, i;

    if (!s->alive) return 0;

    s->dir = s->pending_dir;
    hx = s->body[0].x;
    hy = s->body[0].y;
    if (s->dir == DIR_UP) hy--;
    else if (s->dir == DIR_DOWN) hy++;
    else if (s->dir == DIR_LEFT) hx--;
    else hx++;

    /* Wall hit ends the run. */
    if (hx < 0 || hx >= GRID_W || hy < 0 || hy >= GRID_H) {
        s->alive = 0;
        return 0;
    }

    grow = (s->food.x == hx && s->food.y == hy && s->len < MAX_LEN);
    if (hits_body(s, !grow, hx, hy)) {
        s->alive = 0;
        return 0;
    }

    /* Shift segments toward the tail, then write the new head. When not
     * growing the tail cell is vacated first, when growing the body
     * extends by one segment. */
    newlen = s->len;
    if (!grow) newlen--;
    i = newlen;
    while (i > 0) {
        s->body[i] = s->body[i - 1];
        i--;
    }
    s->body[0].x = hx;
    s->body[0].y = hy;
    s->len = newlen + 1;

    if (s->food.x == hx && s->food.y == hy) {
        s->score += 10;
        if (!game_food_spawn(s)) {
            s->food.x = -1; /* board full: treated as a win by the UI */
        }
    }
    return 1;
}
