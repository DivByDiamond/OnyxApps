/* fileio.c — file load/save. */
#include "vim.h"
/* ── File I/O ──────────────────────────────────────────────────────── */
void load_file(const char *path) {
    FILE *f;
    static char buf[MAX_LINE + 4];
    strncpy(fname, path, sizeof(fname) - 1);
    fname[sizeof(fname) - 1] = 0;
    nlines = 0;
    f = fopen(path, "r");
    if (!f) {
        set_msg("(new file)");
        nlines = 1;
        llen[0] = 0;
        lines[0][0] = 0;
        dirty = 0;
        return;
    }
    while (nlines < MAX_LINES && fgets(buf, sizeof(buf), f)) {
        int len = (int)strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = 0;
        if (len > MAX_LINE - 1) len = MAX_LINE - 1;
        memcpy(lines[nlines], buf, len);
        lines[nlines][len] = 0;
        llen[nlines] = len;
        nlines++;
    }
    fclose(f);
    if (nlines == 0) {
        nlines = 1;
        llen[0] = 0;
        lines[0][0] = 0;
    }
    dirty = 0;
    cx = 0; cy = 0; top = 0;
}

int save_to(const char *path) {
    FILE *f;
    int i;
    if (!path[0]) {
        set_msg("E32: No file name");
        return -1;
    }
    f = fopen(path, "w");
    if (!f) {
        set_msg("E212: Can't open file for writing");
        return -1;
    }
    for (i = 0; i < nlines; i++) {
        fwrite(lines[i], 1, llen[i], f);
        fputc('\n', f);
    }
    fclose(f);
    dirty = 0;
    return 0;
}
