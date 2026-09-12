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
#include <stdarg.h>

int screen_rows = 24;
int screen_cols = 80;

/* Atomic frame buffer: one write() per frame avoids init/log interleaving. */
static char outbuf[8192];
static int outpos = 0;

void obuf_reset(void) { outpos = 0; }
void obuf_printf(const char *fmt, ...) {
    va_list ap;
    int n, cap = (int)sizeof(outbuf) - outpos - 1;
    if (cap <= 0) return;
    va_start(ap, fmt);
    n = vsnprintf(outbuf + outpos, cap + 1, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > cap) n = cap;
    outpos += n;
}
void obuf_flush(void) {
    if (outpos > 0) {
        write(1, outbuf, outpos);
        outpos = 0;
    }
}

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
    /* Alt-screen: isolates TUI from init/login logs that share fd 1. */
    obuf_reset();
    obuf_printf("\x1b[?1049h\x1b[2J\x1b[?25l");
    obuf_flush();

    while (!quit) {
        if (!paused) {
            int y_mem = 3;
            int y_load = 11;
            int y_hist = 17;
            get_size();
            probe_collect(&oi);
            hist_push(&hist, (int)oi.heap_used_kb);
            obuf_reset();
            obuf_printf("\x1b[2J\x1b[H");
            w = screen_cols - 2;
            if (w > 76) w = 76;
            if (w < 20) w = 20;
            /* Compact layout for small OC2R monitors (e.g. GPU T1 40x12). */
            if (screen_rows < 24) {
                y_mem = 2;
                y_load = y_mem + 7;
                y_hist = (screen_rows >= 18) ? y_load + 6 : -1;
            }
            draw_header(&oi, frame);
            panel_memory(y_mem, w, &oi);
            panel_load(y_load, w, &oi);
            if (y_hist >= 0) panel_history(y_hist, w, &hist);
            draw_footer(screen_rows - 1, "q: quit  space: pause  +: slower  -: faster");
            obuf_flush();
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

    obuf_reset();
    obuf_printf("\x1b[?1049l\x1b[?25h\x1b[2J\x1b[H");
    obuf_flush();
    term_raw_off();
    printf("otop: bye\r\n");
    return 0;
}
