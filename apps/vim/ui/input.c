/* input.c - terminal raw mode, size query and key decoding. */
#include "vim.h"
#include "../../lib/term/term.h"

/* ── Input ─────────────────────────────────────────────────────────── */
unsigned char key_pushback;
int have_pushback = 0;

int kbread(unsigned char *out) {
    if (have_pushback) {
        *out = key_pushback;
        have_pushback = 0;
        return 1;
    }
    return (int)read(0, out, 1);
}

int read_key(void) {
    unsigned char c;
    unsigned char s1, s2, s3;
    if (kbread(&c) <= 0) return 0x1b;
    if (c != 0x1b) return c;
    s1 = s2 = s3 = 0;
    if (kbread(&s1) <= 0) return 0x1b;
    if (s1 != '[') {
        /* Not a CSI sequence - return the pushed-back byte next time so
         * ESC immediately followed by a command char works both from
         * terminals (bytes arrive separately) and scripts (piped). */
        key_pushback = s1;
        have_pushback = 1;
        return 0x1b;
    }
    if (kbread(&s2) <= 0) return 0x1b;
    if (s2 >= '0' && s2 <= '9') {
        if (kbread(&s3) <= 0) return 0x1b;
        if (s3 == '~') {
            switch (s2) {
                case '1': return K_HOME;
                case '3': return K_DEL;
                case '4': return K_END;
                case '5': return K_PGUP;
                case '6': return K_PGDN;
            }
        }
        return 0;
    }
    switch (s2) {
        case 'A': return K_UP;
        case 'B': return K_DOWN;
        case 'C': return K_RIGHT;
        case 'D': return K_LEFT;
        case 'H': return K_HOME;
        case 'F': return K_END;
    }
    return 0;
}

/* ── Terminal ──────────────────────────────────────────────────────── */
void raw_enable(void) {
    if (term_raw_enable(&orig_tio)) raw_on = 1;
}

void raw_disable(void) {
    if (raw_on) {
        term_raw_restore(&orig_tio);
        raw_on = 0;
    }
}

void query_size(void) {
    int r = rows, c = cols;
    if (term_get_size(&r, &c) && r > 3 && c > 10) {
        rows = r;
        cols = c;
    }
    if (rows < 8) rows = 8;
    if (cols < 24) cols = 24;
}
