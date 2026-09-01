/*
 * food.c - deterministic LCG RNG and food placement for osnake.
 *
 * The RNG is seeded by game_reset from wall-clock + PID, but any fixed
 * seed reproduces the same food sequence, which is what the host tests
 * rely on. Pure C99, no I/O, no kernel calls.
 */
#include "osnake.h"

/* Classic glibc-style LCG, truncated to 15 usable bits. */
unsigned long rng_next(unsigned long *state) {
    *state = *state * 1103515245 + 12345;
    return (*state >> 16) & 0x7fff;
}

static int on_snake(struct snake *s, int x, int y) {
    int i;
    for (i = 0; i < s->len; i++) {
        if (s->body[i].x == x && s->body[i].y == y) return 1;
    }
    return 0;
}

/* Pick a free cell for the food. Returns 0 when no free cell exists
 * (the board is completely filled with the snake: a win). */
int game_food_spawn(struct snake *s) {
    int x, y, tries;

    if (s->len >= GRID_W * GRID_H) return 0;

    /* Random probes first: cheap and uniform while the board is sparse. */
    for (tries = 0; tries < 4096; tries++) {
        x = (int)(rng_next(&s->rng) % GRID_W);
        y = (int)(rng_next(&s->rng) % GRID_H);
        if (!on_snake(s, x, y)) {
            s->food.x = x;
            s->food.y = y;
            return 1;
        }
    }

    /* Board is nearly full: fall back to a linear scan. */
    for (y = 0; y < GRID_H; y++) {
        for (x = 0; x < GRID_W; x++) {
            if (!on_snake(s, x, y)) {
                s->food.x = x;
                s->food.y = y;
                return 1;
            }
        }
    }
    return 0;
}
