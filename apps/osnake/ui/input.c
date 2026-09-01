/*
 * input.c - terminal control and nonblocking key decoding for osnake.
 *
 * Raw mode via cfmakeraw_apply + VMIN=0/VTIME=0: read() then returns
 * immediately with 0 bytes when no key is pressed, which drives the
 * frame loop without blocking. Arrow keys arrive as CSI sequences.
 */
#include "osnake.h"

int screen_rows = 24;
int screen_cols = 80;

static struct termios orig_termios;
static int raw_on = 0;

void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &orig_termios);
    t = orig_termios;
    cfmakeraw_apply(&t);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, 0, &t);
    raw_on = 1;
}

void term_raw_off(void) {
    if (raw_on) {
        tcsetattr(0, 0, &orig_termios);
        raw_on = 0;
    }
}

/* Real framebuffer grid from TIOCGWINSZ (same probe as oed/osysmon). */
void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    if (_onyx_ioctl(0, 0x5413, (long)ws) == 0 && ws[0] > 2 && ws[1] > 8) {
        screen_rows = ws[0];
        screen_cols = ws[1];
    }
    if (screen_rows < 5) screen_rows = 5;
    if (screen_cols < 20) screen_cols = 20;
}

/* Read one raw byte without blocking; 0 = nothing available. */
static int read_byte(unsigned char *c) {
    long n = read(0, c, 1);
    return n == 1;
}

/* Decode the next key. Plain bytes pass through, CSI A/B/C/D become
 * K_UP/K_DOWN/K_RIGHT/K_LEFT, anything else maps to 0 (ignored). */
int key_poll(void) {
    unsigned char c = 0;
    unsigned char s1 = 0;
    unsigned char s2 = 0;

    if (!read_byte(&c)) return 0;
    if (c != 0x1b) return c;

    if (!read_byte(&s1)) return 0x1b;
    if (s1 != '[') return 0;
    if (!read_byte(&s2)) return 0;
    if (s2 == 'A') return K_UP;
    if (s2 == 'B') return K_DOWN;
    if (s2 == 'C') return K_RIGHT;
    if (s2 == 'D') return K_LEFT;
    return 0;
}
