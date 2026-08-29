/*
 * oed.c — OnyxOS text editor (nano/vim-inspired, single file).
 *
 * Full-screen terminal editor demonstrating the new userspace stack:
 *   - ANSI/VT100 console (colors, cursor addressing)
 *   - termios raw mode (per-character input, no echo)
 *   - libonyxc buffered stdio
 *
 * Features:
 *   - Cursor movement: arrows, Home/End, PageUp/PageDown
 *   - Editing: printable chars, Enter, Backspace, Delete, Tab
 *   - Ctrl+S save, Ctrl+Q quit, Ctrl+G help
 *   - Status bar, line/count display, dirty indicator
 *   - Handles files up to OED_MAX_LINES lines
 *
 * Usage: oed [file]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define OED_MAX_LINES   2048
#define OED_MAX_LINE    512
#define TAB_WIDTH       4

#define KEY_UP      0x100
#define KEY_DOWN    0x101
#define KEY_LEFT    0x102
#define KEY_RIGHT   0x103
#define KEY_HOME    0x104
#define KEY_END     0x105
#define KEY_PGUP    0x106
#define KEY_PGDN    0x107
#define KEY_DEL     0x108

static char lines[OED_MAX_LINES][OED_MAX_LINE];
static int line_len[OED_MAX_LINES];
static int nlines = 0;

static int cx = 0, cy = 0;          /* cursor col/row (text space) */
static int scroll_row = 0;          /* first visible row */
static int screen_rows = 24, screen_cols = 80;
static int dirty = 0;
static char filename[256] = "(new)";
static char status_msg[128] = "";
static struct termios orig_termios;
static int raw_mode_on = 0;

static void term_raw_off(void);

/* ── Terminal control ─────────────────────────────────────────────── */
static void term_raw_on(void) {
    struct termios t;
    tcgetattr(0, &orig_termios);
    t = orig_termios;
    cfmakeraw_apply(&t);
    tcsetattr(0, 0, &t);
    raw_mode_on = 1;
}

static void term_raw_off(void) {
    if (raw_mode_on) {
        tcsetattr(0, 0, &orig_termios);
        raw_mode_on = 0;
    }
}

static void get_size(void) {
    unsigned short ws[4] = {24, 80, 0, 0};
    long r = _onyx_ioctl(0, 0x5413, (long)ws);
    if (r == 0 && ws[0] > 2 && ws[1] > 8) {
        screen_rows = ws[0];
        screen_cols = ws[1];
    }
    if (screen_rows < 5) screen_rows = 5;
    if (screen_cols < 20) screen_cols = 20;
}

/* ── Screen drawing ───────────────────────────────────────────────── */
static void draw_all(void) {
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

static void set_status(const char *msg) {
    strncpy(status_msg, msg, sizeof(status_msg) - 1);
    status_msg[sizeof(status_msg) - 1] = 0;
}

static void place_cursor(void) {
    int srow = cy - scroll_row + 2;
    int scol = cx + 6;
    if (srow < 2) srow = 2;
    printf("\x1b[%d;%dH", srow, scol);
    fflush(stdout);
}

static void scroll_into_view(void) {
    int visible = screen_rows - 2;
    if (cy < scroll_row) scroll_row = cy;
    if (cy >= scroll_row + visible) scroll_row = cy - visible + 1;
}

/* ── Editing ──────────────────────────────────────────────────────── */
static void ensure_line(void) {
    if (nlines == 0) {
        nlines = 1;
        line_len[0] = 0;
    }
    if (cy >= nlines) cy = nlines - 1;
    if (cx > line_len[cy]) cx = line_len[cy];
}

static void insert_char(int c) {
    ensure_line();
    int len = line_len[cy];
    if (len >= OED_MAX_LINE - 1) return;
    for (int i = len; i > cx; i--) {
        lines[cy][i] = lines[cy][i - 1];
    }
    lines[cy][cx] = (char)c;
    line_len[cy]++;
    cx++;
    dirty = 1;
}

static void insert_newline(void) {
    if (nlines >= OED_MAX_LINES) return;
    ensure_line();
    for (int i = nlines; i > cy + 1; i--) {
        memcpy(lines[i], lines[i - 1], OED_MAX_LINE);
        line_len[i] = line_len[i - 1];
    }
    int tail = line_len[cy] - cx;
    if (tail > 0) {
        memcpy(lines[cy + 1], lines[cy] + cx, tail);
    }
    lines[cy + 1][tail] = 0;
    line_len[cy + 1] = tail;
    line_len[cy] = cx;
    lines[cy][cx] = 0;
    nlines++;
    cy++;
    cx = 0;
    dirty = 1;
}

static void backspace(void) {
    if (nlines == 0) return;
    ensure_line();
    if (cx > 0) {
        int len = line_len[cy];
        for (int i = cx - 1; i < len; i++) {
            lines[cy][i] = lines[cy][i + 1];
        }
        line_len[cy]--;
        cx--;
        dirty = 1;
    } else if (cy > 0) {
        int plen = line_len[cy - 1];
        int tlen = line_len[cy];
        if (plen + tlen < OED_MAX_LINE) {
            memcpy(lines[cy - 1] + plen, lines[cy], tlen);
            line_len[cy - 1] = plen + tlen;
        }
        for (int i = cy; i < nlines - 1; i++) {
            memcpy(lines[i], lines[i + 1], OED_MAX_LINE);
            line_len[i] = line_len[i + 1];
        }
        nlines--;
        cy--;
        cx = plen;
        dirty = 1;
    }
}

static void delete_key(void) {
    if (nlines == 0 || cy >= nlines) return;
    if (cx < line_len[cy]) {
        for (int i = cx; i < line_len[cy]; i++) {
            lines[cy][i] = lines[cy][i + 1];
        }
        line_len[cy]--;
        dirty = 1;
    } else if (cy < nlines - 1) {
        int clen = line_len[cy];
        int nlen = line_len[cy + 1];
        if (clen + nlen < OED_MAX_LINE) {
            memcpy(lines[cy] + clen, lines[cy + 1], nlen);
            line_len[cy] = clen + nlen;
        }
        for (int i = cy + 1; i < nlines - 1; i++) {
            memcpy(lines[i], lines[i + 1], OED_MAX_LINE);
            line_len[i] = line_len[i + 1];
        }
        nlines--;
        dirty = 1;
    }
}

/* ── File I/O ─────────────────────────────────────────────────────── */
static void load_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    nlines = 0;
    static char buf[OED_MAX_LINE + 4];
    while (nlines < OED_MAX_LINES && fgets(buf, sizeof(buf), f)) {
        int len = (int)strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = 0;
        }
        if (len > OED_MAX_LINE - 1) len = OED_MAX_LINE - 1;
        memcpy(lines[nlines], buf, len);
        lines[nlines][len] = 0;
        line_len[nlines] = len;
        nlines++;
    }
    fclose(f);
    if (nlines == 0) {
        nlines = 1;
        line_len[0] = 0;
    }
    dirty = 0;
}

static int save_file(void) {
    if (filename[0] == '(') {
        set_status("No filename");
        return -1;
    }
    FILE *f = fopen(filename, "w");
    if (!f) {
        set_status("SAVE FAILED");
        return -1;
    }
    for (int i = 0; i < nlines; i++) {
        fwrite(lines[i], 1, line_len[i], f);
        fputc('\n', f);
    }
    fclose(f);
    dirty = 0;
    set_status("Saved");
    return 0;
}

/* ── Input ────────────────────────────────────────────────────────── */
static int read_key(void) {
    unsigned char c;
    long n = read(0, &c, 1);
    if (n <= 0) return 0x11;
    if (c == 0x1b) {
        unsigned char s1 = 0, s2 = 0, s3 = 0;
        if (read(0, &s1, 1) <= 0) return 0x1b;
        if (s1 != '[') return 0x1b;
        if (read(0, &s2, 1) <= 0) return 0x1b;
        if (s2 >= '0' && s2 <= '9') {
            if (read(0, &s3, 1) <= 0) return 0x1b;
            if (s3 == '~') {
                switch (s2) {
                    case '1': return KEY_HOME;
                    case '3': return KEY_DEL;
                    case '4': return KEY_END;
                    case '5': return KEY_PGUP;
                    case '6': return KEY_PGDN;
                }
            }
            return 0;
        }
        switch (s2) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
        }
        return 0;
    }
    return c;
}

/* ── Help ─────────────────────────────────────────────────────────── */
static void show_help(void) {
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

/* ── Main ─────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    if (argc > 1) {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = 0;
        load_file(filename);
        snprintf(status_msg, sizeof(status_msg), "Opened %s", filename);
    } else {
        nlines = 1;
        line_len[0] = 0;
        strcpy(status_msg, "New file — ^S save ^Q quit ^G help");
    }

    get_size();
    term_raw_on();

    draw_all();
    place_cursor();

    for (;;) {
        int k = read_key();
        switch (k) {
            case 0x11:   /* Ctrl-Q */
                if (dirty) {
                    set_status("Unsaved! Ctrl+Q again to force");
                    draw_all();
                    place_cursor();
                    int k2 = read_key();
                    if (k2 != 0x11) {
                        set_status("");
                        continue;
                    }
                }
                printf("\x1b[2J\x1b[1;1H\x1b[?25h");
                fflush(stdout);
                term_raw_off();
                return 0;
            case 0x13:   /* Ctrl-S */
                save_file();
                break;
            case 0x7:    /* Ctrl-G */
                term_raw_off();
                show_help();
                term_raw_on();
                break;
            case KEY_UP:
                if (cy > 0) cy--;
                if (cx > line_len[cy]) cx = line_len[cy];
                break;
            case KEY_DOWN:
                if (cy < nlines - 1) cy++;
                if (cx > line_len[cy]) cx = line_len[cy];
                break;
            case KEY_LEFT:
                if (cx > 0) cx--;
                break;
            case KEY_RIGHT:
                if (cx < line_len[cy]) cx++;
                break;
            case KEY_HOME:
                cx = 0;
                break;
            case KEY_END:
                cx = line_len[cy];
                break;
            case KEY_PGUP:
                cy -= (screen_rows - 3);
                if (cy < 0) cy = 0;
                break;
            case KEY_PGDN:
                cy += (screen_rows - 3);
                if (cy > nlines - 1) cy = nlines - 1;
                break;
            case KEY_DEL:
                delete_key();
                break;
            case '\r':
            case '\n':
                insert_newline();
                break;
            case 0x7f:
            case 0x08:
                backspace();
                break;
            case '\t':
                for (int i = 0; i < TAB_WIDTH; i++) insert_char(' ');
                break;
            default:
                if (k >= 0x20 && k < 0x7f) {
                    insert_char(k);
                }
                break;
        }
        scroll_into_view();
        draw_all();
        place_cursor();
    }
}
