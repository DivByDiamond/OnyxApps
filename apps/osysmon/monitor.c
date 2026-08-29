/*
 * monitor.c - system probes and drawing primitives for osysmon.
 *
 * Data collection goes through libonyxc syscalls (uname, sbrk,
 * CLOCK_MONOTONIC, readdir); rendering is plain ANSI box drawing.
 */
#include "osysmon.h"

/* Fill a utsname buffer via the kernel uname syscall. */
void sysinfo_get(struct utsname_l *un) {
    memset(un, 0, sizeof(*un));
    _onyx_uname(un);
}

/* Milliseconds since boot, from CLOCK_MONOTONIC. */
long uptime_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Heap estimate: sbrk(0) relative to a fixed base. */
long heap_usage_kb(void) {
    static long heap_base = 0;
    long brk_now = (long)_onyx_sbrk(0);
    if (heap_base == 0) heap_base = brk_now;
    return (brk_now - heap_base) / 1024;
}

/* Count entries in a directory via readdir syscall. */
int count_dir(const char *path) {
    char namebuf[256];
    int count = 0;
    /* _onyx_readdir(dir, name_out, len) enumerates one name per call,
     * returning 1 while entries remain. */
    for (;;) {
        long r = _onyx_readdir(path, namebuf, sizeof(namebuf));
        if (r <= 0) break;
        count++;
        if (count > 4096) break;
    }
    return count;
}

void bar(int row, const char *label, int permille, const char *suffix) {
    printf("\x1b[%d;3H%s", row, label);
    int filled = (permille * BAR_W) / 1000;
    if (filled > BAR_W) filled = BAR_W;
    if (filled < 0) filled = 0;
    /* Color by load: green < 50%, yellow < 80%, red above. */
    const char *col = permille < 500 ? "\x1b[32m"
                    : permille < 800 ? "\x1b[33m" : "\x1b[31m";
    printf("\x1b[%d;16H[", row);
    printf("%s", col);
    for (int i = 0; i < BAR_W; i++) {
        putchar(i < filled ? '#' : ' ');
    }
    printf("\x1b[0m] %s", suffix);
}

void box(int r1, int c1, int r2, int c2, const char *title) {
    /* Border with plain ASCII (font-safe). */
    printf("\x1b[%d;%dH+", r1, c1);
    for (int c = c1 + 1; c < c2; c++) putchar('-');
    printf("+");
    for (int r = r1 + 1; r < r2; r++) {
        printf("\x1b[%d;%dH|", r, c1);
        printf("\x1b[%d;%dH|", r, c2);
    }
    printf("\x1b[%d;%dH+", r2, c1);
    for (int c = c1 + 1; c < c2; c++) putchar('-');
    printf("+");
    if (title && title[0]) {
        printf("\x1b[%d;%dH\x1b[1;36m %s \x1b[0m", r1, c1 + 2, title);
    }
}
