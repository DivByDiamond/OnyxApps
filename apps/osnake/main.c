/*
 * main.c - program entry and the frame loop for osnake.
 *
 * Usage: osnake [tick-ms]
 *
 * Each frame drains pending keys (last direction wins), advances the
 * simulation unless paused, redraws the board and sleeps for the tick
 * interval. Speed ramps up with the score unless an explicit interval
 * is given on the command line.
 */
#include "osnake.h"

#define BASE_MS 160 /* starting tick when auto speed is on */
#define MIN_MS 70   /* fastest auto tick */

static int auto_ms = 1; /* 1 = speed derived from score */

static int tick_ms(int score) {
    int ms;
    if (!auto_ms) return 0; /* caller uses its own interval */
    ms = BASE_MS - (score / 10) * 4;
    return ms < MIN_MS ? MIN_MS : ms;
}

static void nap(int ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

static int dir_from_key(int k) {
    if (k == K_UP || k == 'w') return DIR_UP;
    if (k == K_DOWN || k == 's') return DIR_DOWN;
    if (k == K_LEFT || k == 'a') return DIR_LEFT;
    if (k == K_RIGHT || k == 'd') return DIR_RIGHT;
    return -1;
}

static unsigned long make_seed(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec ^ (unsigned long)ts.tv_nsec ^
           (unsigned long)getpid() ^ 0x9e3779b9UL;
}

/* Apply one decoded key; returns 1 when the game should quit. */
static int handle_key(struct snake *game, int k, int *paused) {
    int d = dir_from_key(k);
    if (d >= 0) {
        game_set_dir(game, d);
        return 0;
    }
    if (k == 'q' || k == 'Q' || k == 0x11 || k == 0x03) return 1;
    if (k == ' ' || k == 'p' || k == 'P') *paused = !(*paused);
    return 0;
}

int main(int argc, char **argv) {
    struct snake game;
    int interval = 0;
    int quit = 0;
    int paused = 0;

    if (argc > 1) {
        interval = atoi(argv[1]);
        if (interval < 30) interval = 30;
        auto_ms = 0;
    }

    get_size();
    term_raw_on();
    game_reset(&game, make_seed());

    printf("\x1b[?25l"); /* hide cursor */
    while (game.alive && !quit) {
        int k;
        for (k = key_poll(); k != 0; k = key_poll()) {
            if (handle_key(&game, k, &paused)) quit = 1;
        }
        if (quit) break;
        if (!paused) game_step(&game);
        if (game.alive) {
            render_frame(&game, paused);
            nap(auto_ms ? tick_ms(game.score) : interval);
        }
    }

    if (!quit && !game.alive) {
        int won = (game.len >= MAX_LEN);
        int k;
        render_over(&game, won);
        for (;;) {
            nap(30);
            k = key_poll();
            if (k != 0) break; /* any key exits */
        }
    }

    printf("\x1b[2J\x1b[1;1H\x1b[?25h");
    fflush(stdout);
    term_raw_off();
    printf("osnake: final score %d, bye\r\n", game.score);
    return 0;
}
