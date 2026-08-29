/*
 * screen.c - terminal control (raw mode, window size) and screen
 * rendering for oed.
 */
#include "oed.h"

/* Terminal state */
static struct termios orig_termios;
static int raw_mode_on = 0;
static int scroll_row = 0;    /* first visible row */
int screen_rows = 24, screen_cols = 80;

/* Terminal control */
void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &orig_termios);
    t = orig_termios;
    cfmakeraw_apply(&t);
    tcsetattr(0, 0, &t);
    raw_mode_on = 1;
}

void term_raw_off(void) {
    if (raw_mode_on) {
        tcsetattr(0, 0, &orig_termios);
        raw_mode_on = 0;
    }
}

void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    long r = _onyx_ioctl(0, 0x5413, (long)ws);
    if (r == 0 && ws[0] > 2 && ws[1] > 8) {
        screen_rows = ws[0];
        screen_cols = ws[1];
    }
    if (screen_rows < 5) screen_rows = 5;
    if (screen_cols < 20) screen_cols = 20;
}

/* Screen drawing */
void draw_all(void) {
    printf("\x1b[2J");
    printf("\x1b[1;1H\x1b[7m oed — %s %s\x1b[0m\x1b[K",
           filename, dirty ? "(modified)" : "");
    int visible = screen_rows - 2;
    for (int i = 0; i < visible; i++) {
        int row = scroll_row + i;
        if (row < nlines) {
            printf("\x1b[%d;1H\x1b[36m%4d \x1b[0m\x1b[K", i + 2, row + 1);
            int maxch = screen_cols - 6;
            if (maxch < 1) maxch = 1;
            int len = line_len[row];
            if (len > maxch) len = maxch;
            if (len > 0) fwrite(lines[row], 1, len, stdout);
        } else {
            printf("\x1b[%d;1H\x1b[90m    ~\x1b[0m\x1b[K", i + 2);
        }
    }
    printf("\x1b[%d;1H\x1b[7m %-60s\x1b[0m", screen_rows - 1, status_msg);
    printf(" \x1b[7mL%d/%d C%d\x1b[0m\x1b[K", cy + 1, nlines, cx + 1);
    fflush(stdout);
}

void set_status(const char *msg) {
    strncpy(status_msg, msg, sizeof(status_msg) - 1);
    status_msg[sizeof(status_msg) - 1] = 0;
}

void place_cursor(void) {
    int srow = cy - scroll_row + 2;
    int scol = cx + 6;
    if (srow < 2) srow = 2;
    printf("\x1b[%d;%dH", srow, scol);
    fflush(stdout);
}

void scroll_into_view(void) {
    int visible = screen_rows - 2;
    if (cy < scroll_row) scroll_row = cy;
    if (cy >= scroll_row + visible) scroll_row = cy - visible + 1;
}

/* Help */
void show_help(void) {
    printf("\x1b[2J\x1b[1;1H\x1b[1;7m oed — help \x1b[0m\r\n\r\n");
    printf("Ctrl+S     Save file        Ctrl+Q  Quit\r\n");
    printf("Ctrl+G     This help\r\n\r\n");
    printf("Arrows     Move cursor       Home/End  Line start/end\r\n");
    printf("PgUp/PgDn  Page up/down\r\n\r\n");
    printf("Enter      New line          Tab    Insert %d spaces\r\n", TAB_WIDTH);
    printf("Backspace  Delete left       Delete Delete right\r\n\r\n");
    printf("Press any key...\r\n");
    fflush(stdout);
    read_key();
}
