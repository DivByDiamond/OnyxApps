/*
 * parse.c - pure parsers and formatting helpers for otop, no I/O.
 *
 * The /proc file formats reproduced here match OnyxKernel
 * kernel/src/fs/procfs exactly, e.g.:
 *
 *   MemTotal\t: 262144 kB\n
 *   processes\t: 7\n
 *   123.45\n            (uptime)
 *
 * Everything in this file is plain C99 and is exercised by the host
 * tests in tests/native/ without OnyxOS or onyxcc.
 */
#include "otop.h"

/* Find `key` at the start of a line; NULL when absent. */
static const char *find_key(const char *buf, const char *key) {
    const char *p = buf;

    while ((p = strstr(p, key)) != NULL) {
        if (p == buf || p[-1] == '\n') return p;
        p++;
    }
    return NULL;
}

/* Parse the integer that follows "key" on its line ("MemTotal : 123 kB").
 * Returns 1 on success and stores the value in *out, 0 otherwise. */
int parse_num(const char *buf, const char *key, long *out) {
    const char *p = find_key(buf, key);
    long v = 0;

    if (!p) return 0;
    while (*p && (*p < '0' || *p > '9')) {
        if (*p == '\n') return 0; /* no number on this line */
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        p++;
    }
    *out = v;
    return 1;
}

/* /proc/uptime is "SECONDS.CENTIS"; keep the integer part. */
long parse_uptime_s(const char *buf) {
    const char *p = buf;
    long v = 0;

    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        p++;
    }
    return v;
}

/* used/total in permille (0..1000), safe against zero total and overflow. */
int permille(long used, long total) {
    long p;

    if (total <= 0) return 0;
    if (used < 0) used = 0;
    p = used * 1000 / total;
    if (p > 1000) p = 1000;
    return (int)p;
}

/* "HH:MM:SS" into out (needs cap >= 9). */
void fmt_uptime(long s, char *out, int cap) {
    if (cap < 9) {
        if (cap > 0) out[0] = 0;
        return;
    }
    snprintf(out, cap, "%02ld:%02ld:%02ld", s / 3600, (s / 60) % 60, s % 60);
}

/* Append a sample; once full the oldest value is dropped (ring shift). */
void hist_push(struct otop_hist *h, int val) {
    int i;

    if (h->n < OTOP_HIST) {
        h->v[h->n] = val;
        h->n++;
        return;
    }
    for (i = 0; i < OTOP_HIST - 1; i++) h->v[i] = h->v[i + 1];
    h->v[OTOP_HIST - 1] = val;
}

/* Sparkline glyph for a 0..1000 value: 9 ASCII levels. */
char spark_ch(int level_permille) {
    static char glyphs[9] = {' ', '.', ':', '-', '=', '+', '*', '#', '@'};

    if (level_permille < 0) level_permille = 0;
    if (level_permille > 1000) level_permille = 1000;
    return glyphs[level_permille * 9 / 1001];
}

/* Render the last `width` samples of the history as a sparkline into
 * out (which must hold width + 1 bytes). Scaling is relative to the
 * maximum of the rendered window; missing samples render as spaces. */
void spark_line(struct otop_hist *h, char *out, int width) {
    int vmax = 1;
    int i, j, first;

    if (width > OTOP_HIST) width = OTOP_HIST;
    if (width < 1) width = 1;
    first = h->n - width;
    if (first < 0) first = 0;

    for (i = first; i < h->n; i++) {
        if (h->v[i] > vmax) vmax = h->v[i];
    }
    for (j = 0; j < width; j++) {
        i = first + j;
        if (i >= h->n) {
            out[j] = ' ';
        } else {
            out[j] = spark_ch(h->v[i] * 1000 / vmax);
        }
    }
    out[width] = 0;
}
