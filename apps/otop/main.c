/*
 * main.c - program entry and the refresh loop for otop.
 *
 * Usage: otop [interval-ms]
 *
 * Every frame: probe the system (probe/data.c), push the heap sample
 * into the history, draw the panels (ui/). Keys: q quit, space pause
 * (freezes the frame and the history), +/- change the interval.
 */
#include "otop.h"

int screen_rows = 24;
int screen_cols = 80;

static struct termios orig_termios;
static int raw_on = 0;

#define DEFAULT_MS 1000
#define MIN_MS 100
#define MAX_MS 5000

void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &orig_termios);
    t = orig_termios;
    cfmakeraw_apply(&t);
    tcsetattr(0, 0, &t);
    raw_on = 1;
}

void term_raw_off(void) {
    if (raw_on) {
        tcsetattr(0, 0, &orig_termios);
        raw_on = 0;
    }
}

void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    if (_onyx_ioctl(0, 0x5413, (long)ws) == 0 && ws[0] > 2 && ws[1] > 8) {
        screen_rows = ws[0];
        screen_cols = ws[1];
    }
    if (screen_rows < 5) screen_rows = 5;
    if (screen_cols < 20) screen_cols = 20;
}

int key_poll(void) {
    unsigned char c = 0;
    long n = read(0, &c, 1);
    if (n <= 0) return 0;
    return c;
}

void nap_ms(int ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

/* Handle one key; returns 1 to quit, stores side effects in state. */
static int handle_key(int k, int *paused, int *interval) {
    if (k == 'q' || k == 'Q' || k == 0x11 || k == 0x03) return 1;
    if (k == ' ') *paused = !(*paused);
    if (k == '+' || k == '=') {
        *interval += 100;
        if (*interval > MAX_MS) *interval = MAX_MS;
    }
    if (k == '-' || k == '_') {
        *interval -= 100;
        if (*interval < MIN_MS) *interval = MIN_MS;
    }
    return 0;
}

int main(int argc, char **argv) {
    struct otop_info oi;
    struct otop_hist hist;
    int interval = DEFAULT_MS;
    int paused = 0;
    int frame = 0;
    int quit = 0;
    int i, k;
    int w;

    if (argc > 1) {
        interval = atoi(argv[1]);
        if (interval < MIN_MS) interval = MIN_MS;
        if (interval > MAX_MS) interval = MAX_MS;
    }

    memset(&hist, 0, sizeof(hist));
    get_size();
    term_raw_on();

    while (!quit) {
        if (!paused) {
            probe_collect(&oi);
            hist_push(&hist, (int)oi.heap_used_kb);
            printf("\x1b[2J");
            w = screen_cols - 2;
            if (w > 76) w = 76;
            draw_header(&oi, frame);
            panel_memory(3, w, &oi);
            panel_load(11, w, &oi);
            panel_history(17, w, &hist);
            draw_footer(screen_rows - 1, "q: quit  space: pause  +: slower  -: faster");
            fflush(stdout);
            frame++;
        }

        /* Sleep in slices so quit/pause react fast. */
        for (i = 0; i < interval / 100; i++) {
            nap_ms(100);
            while ((k = key_poll()) != 0) {
                if (handle_key(k, &paused, &interval)) quit = 1;
            }
            if (quit || !paused) break;
        }
        /* When unpausing, refresh immediately on the next pass. */
    }

    printf("\x1b[2J\x1b[1;1H\x1b[?25h");
    fflush(stdout);
    term_raw_off();
    printf("otop: bye\r\n");
    return 0;
}
