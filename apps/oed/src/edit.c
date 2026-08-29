/*
 * edit.c - buffer state, editing primitives and file I/O for oed.
 */
#include "oed.h"

/* Buffer and editor state (extern in oed.h) */
char lines[OED_MAX_LINES][OED_MAX_LINE];
int  line_len[OED_MAX_LINES];
int  nlines = 0;

int  cx = 0, cy = 0;          /* cursor col/row (text space) */
int  dirty = 0;
char filename[256] = "(new)";
char status_msg[128] = "";

/* Editing */
static void ensure_line(void) {
    if (nlines == 0) {
        nlines = 1;
        line_len[0] = 0;
    }
    if (cy >= nlines) cy = nlines - 1;
    if (cx > line_len[cy]) cx = line_len[cy];
}

void insert_char(int c) {
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

void insert_newline(void) {
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

void backspace(void) {
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

void delete_key(void) {
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

/* File I/O */
void load_file(const char *path) {
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

int save_file(void) {
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
