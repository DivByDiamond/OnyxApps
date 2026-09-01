/*
 * osnake.h - shared types and module API for the snake game.
 *
 * main.c       - program entry: args, RNG seed, frame loop, game over
 * game/logic.c - pure state transitions: reset, steering, step
 * game/food.c  - pure LCG RNG and food placement
 * ui/input.c   - terminal raw mode, size probe, nonblocking key decode
 * ui/render.c  - ANSI board drawing and the game over overlay
 *
 * game/ modules are pure C99 with no I/O: they are exercised by the
 * host tests in tests/native/ without OnyxOS or onyxcc.
 */
#ifndef OSNAKE_H
#define OSNAKE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

/* Board grid in cells; the board is drawn 1:1 as characters. */
#define GRID_W 32
#define GRID_H 16
#define MAX_LEN 512 /* == GRID_W * GRID_H: full board is a win */

/* Directions; opposite pairs differ by XOR with 2. */
#define DIR_UP 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_LEFT 3

/* Decoded keys that do not fit in a byte. */
#define K_UP 1000
#define K_DOWN 1001
#define K_RIGHT 1002
#define K_LEFT 1003

struct point {
    int x, y;
};

struct snake {
    struct point body[MAX_LEN]; /* body[0] is the head */
    int len;
    int dir;         /* committed direction of the last step */
    int pending_dir; /* queued turn, applied by the next step */
    struct point food;
    unsigned long rng; /* LCG state, seeded from game_reset */
    int score;         /* +10 per food */
    int alive;
};

/* game/logic.c - pure state transitions. */
void game_reset(struct snake *s, unsigned long seed);
void game_set_dir(struct snake *s, int dir);
int game_step(struct snake *s); /* returns 1 while alive, 0 on game over */

/* game/food.c - RNG and placement. */
unsigned long rng_next(unsigned long *state);
int game_food_spawn(struct snake *s); /* returns 0 when the board is full */

/* ui/input.c - terminal control and nonblocking keys. */
void term_raw_on(void);
void term_raw_off(void);
void get_size(void);
int key_poll(void); /* next decoded key, or 0 when nothing is pending */

/* ui/render.c - drawing. */
void render_frame(struct snake *s, int paused);
void render_over(struct snake *s, int won);

extern int screen_rows;
extern int screen_cols;

#endif
