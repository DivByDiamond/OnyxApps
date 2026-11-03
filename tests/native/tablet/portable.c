#include "config/panel.h"
#include "device/display.h"
#include "service/session.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static int queued;
static int enqueue(void *ctx, unsigned long id, int action, unsigned long deadline) {
    (void)ctx; assert(id && action == 1 && deadline == 100); queued++; return 1;
}
int main(void) {
    struct ow_view v, before;
    struct ow_session s;
    struct ow_transport t;
    struct ow_screen screen;
    unsigned long id;
    int x, y, i;
    t.ctx = 0; t.enqueue = enqueue;
    ow_session_init(&s, &t);
    assert(!ow_session_send(&s, 1, 1, 100));
    ow_session_online(&s, 1);
    id = ow_session_send(&s, 1, 1, 100);
    assert(id == 1 && queued == 1 && ow_session_find(&s, id)->state == OW_QUEUED);
    assert(ow_session_result(&s, id, OW_ACCEPTED));
    assert(ow_session_result(&s, id, OW_COMPLETED));
    assert(!ow_session_result(&s, id, OW_FAILED));
    for (i = 0; i < OW_REQUESTS; i++) assert(ow_session_send(&s, 1, 1, 100));
    assert(!ow_session_send(&s, 1, 1, 100));
    ow_session_tick(&s, 100);
    assert(!ow_session_result(&s, id + 1, OW_COMPLETED));
    id = ow_session_send(&s, 1, 1, 100);
    ow_session_online(&s, 0);
    assert(ow_session_find(&s, id)->state == OW_DISCONNECTED);
    ow_session_online(&s, 1);
    assert(!ow_session_result(&s, id, OW_COMPLETED));
    ow_init(&v, 240, 112);
    assert(ow_panel_load(&v, "button 0 0 20 20 1 3 |CUSTOM\n"));
    assert(v.count == 1 && ow_pointer(&v, 1, 1) == 3);
    before = v;
    assert(!ow_panel_load(&v, "label 9999999999999 0 1 1 0 0 |BAD"));
    assert(!memcmp(&v, &before, sizeof(v)));
    screen.width = 1280; screen.height = 720; screen.bpp = 32;
    screen.pitch = 5120; screen.size = 3686400;
    assert(ow_screen_pointer(&screen, 40, 80, &x, &y) && x == 0 && y == 0);
    assert(!ow_screen_pointer(&screen, 0, 0, &x, &y));
    assert(ow_screen_rgb(0xf800) == 0xff0000);
    puts("tablet: portable panel, screen and async session passed");
    return 0;
}
