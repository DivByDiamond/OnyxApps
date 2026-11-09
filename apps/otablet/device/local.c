#define _POSIX_C_SOURCE 200809L
/* Local running-process data: no fabricated remote machine or energy values. */
#include "display.h"
#include <stdio.h>
#include <time.h>
static unsigned long clicks;
static int sample(void *ctx, struct dashboard_sample *s) {
    struct timespec now;
    (void)ctx;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0;
    s->energy = clicks % 101;
    snprintf(s->machine, sizeof(s->machine), "UP %luS CLICKS %lu", (unsigned long)now.tv_sec, clicks);
    return 1;
}
static int click(void *ctx, int action, unsigned long request) {
    (void)ctx; (void)request;
    if (action != 1) return 0;
    clicks++;
    printf("OTABLET_CLICK=%lu\n", clicks);
    fflush(stdout);
    return 2;
}
void dashboard_local(struct ow_view *v, struct dashboard_backend *b, int layout) {
    clicks = 0;
    b->ctx = 0; b->fetch = sample; b->action = click;
    if (layout) {
        dashboard_init(v);
        ow_update(v, 0, "ONYX TABLET / LOCAL", 0);
        ow_update(v, 4, "ADD CLICK", 0);
        ow_update(v, 5, "Q / ESC / RIGHT: EXIT", 0);
    }
}
