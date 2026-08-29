/*
 * main.c - key decoding and the editor main loop for oed.
 */
#include "oed.h"

/* Input */
int read_key(void) {
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

/* Main */
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
