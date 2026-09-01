/*
 * test_osnake.c - host tests for the pure osnake modules
 * (apps/osnake/game/logic.c + food.c). Runs on any Linux with gcc:
 *
 *   make -C tests/native test_osnake && ./tests/native/test_osnake
 *
 * The tested code has no I/O, so no terminal is needed. Interactive
 * behavior (raw input, ANSI output) is verified manually in QEMU and
 * built for real by CI with onyxcc.
 */
#include <stdio.h>
#include <string.h>
#include "osnake.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void test_reset(void) {
    struct snake s;

    printf("reset invariants:\n");
    game_reset(&s, 42);
    CHECK(s.len == 3);
    CHECK(s.alive == 1);
    CHECK(s.dir == DIR_RIGHT);
    CHECK(s.pending_dir == DIR_RIGHT);
    CHECK(s.score == 0);
    CHECK(s.food.x >= 0 && s.food.x < GRID_W);
    CHECK(s.food.y >= 0 && s.food.y < GRID_H);
    CHECK(!(s.food.x == s.body[0].x && s.food.y == s.body[0].y));
}

static void test_steering(void) {
    struct snake s;

    printf("180 degree reversal is rejected:\n");
    game_reset(&s, 7);
    game_set_dir(&s, DIR_LEFT); /* opposite of DIR_RIGHT */
    CHECK(s.pending_dir == DIR_RIGHT);
    game_set_dir(&s, DIR_UP);
    CHECK(s.pending_dir == DIR_UP);
    game_set_dir(&s, 12345); /* out of range: ignored */
    CHECK(s.pending_dir == DIR_UP);
}

static void test_walls(void) {
    struct snake s;

    printf("wall death:\n");
    game_reset(&s, 7);
    s.body[0].x = GRID_W - 1;
    s.food.x = 0;
    s.food.y = 0;
    s.pending_dir = DIR_RIGHT;
    CHECK(game_step(&s) == 0);
    CHECK(s.alive == 0);
}

static void test_self_collision(void) {
    struct snake s;

    printf("self death:\n");
    game_reset(&s, 7);
    s.body[0].x = 5; s.body[0].y = 5;
    s.body[1].x = 6; s.body[1].y = 5;
    s.body[2].x = 7; s.body[2].y = 5;
    s.len = 3;
    s.dir = DIR_RIGHT;
    s.pending_dir = DIR_RIGHT;
    s.food.x = 0; s.food.y = 0;
    CHECK(game_step(&s) == 0);
    CHECK(s.alive == 0);
}

static void test_growth(void) {
    struct snake s;
    int old_len;

    printf("food growth and scoring:\n");
    game_reset(&s, 7);
    old_len = s.len;
    s.food.x = s.body[0].x + 1; /* right in front of the head */
    s.food.y = s.body[0].y;
    CHECK(game_step(&s) == 1);
    CHECK(s.len == old_len + 1);
    CHECK(s.score == 10);
    CHECK(s.food.x != s.body[0].x || s.food.y != s.body[0].y);
}

static void test_full_board(void) {
    struct snake s;
    int x, y, i;

    printf("full board: spawn fails (win path):\n");
    game_reset(&s, 7);
    i = 0;
    for (y = 0; y < GRID_H; y++) {
        for (x = 0; x < GRID_W; x++) {
            if (i >= MAX_LEN - 1) break;
            s.body[i].x = x;
            s.body[i].y = y;
            i++;
        }
    }
    s.len = i; /* every cell except the last one */
    CHECK(game_food_spawn(&s) == 1);
    CHECK(s.food.x == GRID_W - 1 && s.food.y == GRID_H - 1);
    s.body[i].x = GRID_W - 1;
    s.body[i].y = GRID_H - 1;
    s.len = i + 1; /* board completely full */
    CHECK(game_food_spawn(&s) == 0);
}

static void test_rng(void) {
    unsigned long ra = 42;
    unsigned long rb = 42;
    int ok = 1;
    int i;

    printf("rng determinism:\n");
    for (i = 0; i < 5; i++) {
        if (rng_next(&ra) != rng_next(&rb)) ok = 0;
    }
    CHECK(ok == 1);
    CHECK(ra == rb);
}

static void test_fuzz(void) {
    struct snake s;
    int i, j, ok = 1;

    printf("fuzz: 30 runs x 100 steps with invariants:\n");
    for (i = 0; i < 30; i++) {
        game_reset(&s, (unsigned long)(i + 1) * 7919);
        for (j = 0; j < 100 && s.alive; j++) {
            unsigned long r = (unsigned long)(i * 131 + j * 17);
            if (j % 13 == 0) game_set_dir(&s, (int)(r % 4));
            game_step(&s);
            if (s.len < 3 || s.len > MAX_LEN) ok = 0;
            if (s.score < 0 || s.score % 10 != 0) ok = 0;
            if (s.alive) {
                if (s.body[0].x < 0 || s.body[0].x >= GRID_W) ok = 0;
                if (s.body[0].y < 0 || s.body[0].y >= GRID_H) ok = 0;
            }
        }
    }
    CHECK(ok == 1);
}

int main(void) {
    test_reset();
    test_steering();
    test_walls();
    test_self_collision();
    test_growth();
    test_full_board();
    test_rng();
    test_fuzz();
    printf("\ntest_osnake: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
