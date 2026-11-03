#include "dashboard.h"
#include "graphics/raster.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static unsigned short guarded[102];
static unsigned int glyph(unsigned char c, int row) { return 31; }
static int fetch_mode;
static int fetch(void *ctx, struct dashboard_sample *s) {
    s->energy = fetch_mode == 1 ? 101 : 42;
    strcpy(s->machine, "READY");
    if (fetch_mode == 2) memset(s->machine, 'X', OW_TEXT);
    return fetch_mode != 3;
}
static int accepted(void *ctx, int action, unsigned long request) { return 1; }
static void state_test(void) {
    struct ow_view v, before;
    int i;
    assert(!ow_init(&v, 0, 10));
    assert(ow_init(&v, 10, 10));
    assert(ow_add(&v, OW_BUTTON, -2, -2, 6, 6, 1, 9, "RUN") == 0);
    assert(ow_pointer(&v, 0, 0) == 9);
    assert(!ow_pointer(&v, -1, 0));
    assert(!ow_pointer(&v, 4, 0));
    assert(!ow_pointer(&v, INT_MAX, INT_MAX));
    assert(ow_add(&v, OW_LABEL, 0, 0, 1, 1, 2, 0, "X") == 1);
    assert(!ow_pointer(&v, 0, 0)); /* Topmost label occludes button. */
    assert(ow_add(&v, OW_PROGRESS, 0, 4, 10, 1, 1, 0, "") == 2);
    before = v;
    assert(!ow_update(&v, 1, "new", 101));
    assert(!memcmp(&v, &before, sizeof(v)));
    assert(!ow_update(&v, 1, "\033[2J", 50));
    assert(ow_update(&v, 1, "new", 50));
    assert(v.widgets[0].value == 50 && v.widgets[2].value == 50);
    assert(!ow_update(&v, 99, "unknown", 0));
    assert(ow_add(&v, OW_LABEL, INT_MAX, 0, 1, 1, 0, 0, "") == -1);
    for (i = v.count; i < OW_MAX; i++)
        assert(ow_add(&v, OW_LABEL, 0, 0, 1, 1, 0, 0, "") == i);
    assert(ow_add(&v, OW_LABEL, 0, 0, 1, 1, 0, 0, "") == -1);
    ow_tick(&v, 0, 5); assert(!v.online);
    ow_refresh(&v, 10); ow_tick(&v, 14, 5); assert(v.online);
    ow_tick(&v, 15, 5); assert(!v.online);
    ow_tick(&v, 9, 5); assert(!v.online);
}
static void raster_test(void) {
    struct ow_view v;
    struct ow_raster r;
    int i;
    for (i = 0; i < 102; i++) guarded[i] = 0xbeef;
    assert(!ow_raster_init(&r, guarded + 1, 10, 10, 9, glyph));
    assert(ow_raster_init(&r, guarded + 1, 10, 10, 10, glyph));
    ow_init(&v, 10, 10);
    ow_add(&v, OW_LABEL, -2, -2, 4, 4, 1, 0, "XXX");
    ow_add(&v, OW_PROGRESS, 8, 8, 20, 20, 2, 0, "");
    ow_update(&v, 2, "", 50);
    ow_raster_paint(&v, &r);
    assert(guarded[0] == 0xbeef && guarded[101] == 0xbeef);
    assert(guarded[1] == 0xffff);
    assert(guarded[3] == 0xbeef); /* Text clip ends at x=2. */
    assert(guarded[89] == 0x07e0);
    /* View can exceed destination; raster independently clips writes. */
    ow_init(&v, 100, 100);
    ow_add(&v, OW_BUTTON, 0, 0, 100, 100, 0, 1, "X");
    ow_raster_paint(&v, &r);
    assert(guarded[0] == 0xbeef && guarded[101] == 0xbeef);
}
static void backend_test(void) {
    struct ow_view v, before;
    struct dashboard_backend b;
    int i;
    b.ctx = 0; b.fetch = fetch; b.action = accepted;
    dashboard_init(&v);
    assert(!dashboard_click(&v, &b, 10, 70, 1));
    assert(dashboard_refresh(&v, &b, 10));
    assert(v.online && v.widgets[3].value == 42);
    before = v;
    for (i = 1; i < 4; i++) {
        fetch_mode = i;
        assert(!dashboard_refresh(&v, &b, 11));
        assert(!memcmp(&v, &before, sizeof(v)));
    }
    assert(dashboard_click(&v, &b, 10, 70, 1) == 1);
    assert(!strcmp(v.widgets[5].text, "ACCEPTED / PENDING"));
    assert(!dashboard_click(&v, &b, 10, 70, 0));
    ow_tick(&v, 20, 5);
    assert(!dashboard_click(&v, &b, 10, 70, 2));
}
int main(void) {
    state_test(); raster_test(); backend_test();
    puts("tablet: state, binding, limits, clipping, pointer and backend tests passed");
    return 0;
}
