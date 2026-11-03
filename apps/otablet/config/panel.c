#include "panel.h"
#include <string.h>
/* Explicit bounded decimal parsing: no sscanf integer overflow or allocation. */
static int number(const char **p, int *out) {
    int n = 0, neg = 0, digits = 0;
    while (**p == ' ') (*p)++;
    if (**p == '-') { neg = 1; (*p)++; }
    while (**p >= '0' && **p <= '9') {
        n = n * 10 + *(*p)++ - '0';
        if (n > OW_EXTENT) return 0;
        digits++;
    }
    if (!digits || **p != ' ') return 0;
    *out = neg ? -n : n;
    return 1;
}
int ow_panel_load(struct ow_view *v, const char *source) {
    struct ow_view next;
    const char *p = source;
    char kind[9], text[OW_TEXT];
    int a[6], i, n, type, lines = 0;
    if (!v || !p || !ow_init(&next, v->width, v->height)) return 0;
    for (i = 0; i <= 2048 && source[i]; i++) {}
    if (i > 2048) return 0;
    while (*p) {
        if (++lines > OW_MAX) return 0;
        n = 0;
        while (*p && *p != ' ') {
            if (n == 8) return 0;
            kind[n++] = *p++;
        }
        kind[n] = 0;
        type = !strcmp(kind, "label") ? OW_LABEL : !strcmp(kind, "button") ? OW_BUTTON :
            !strcmp(kind, "progress") ? OW_PROGRESS : !strcmp(kind, "status") ? OW_STATUS : -1;
        if (type < 0) return 0;
        for (i = 0; i < 6; i++) if (!number(&p, &a[i])) return 0;
        while (*p == ' ') p++;
        if (*p++ != '|') return 0;
        n = 0;
        while (*p && *p != '\n') {
            if (n >= OW_TEXT - 1 || (unsigned char)*p < 32 || (unsigned char)*p > 126) return 0;
            text[n++] = *p++;
        }
        text[n] = 0;
        if (*p == '\n') p++;
        if (ow_add(&next, type, a[0], a[1], a[2], a[3], a[4], a[5], text) < 0) return 0;
    }
    if (!next.count) return 0;
    *v = next;
    return 1;
}
