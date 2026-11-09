#include "device/display.h"
#include "config/panel.h"
#include "service/session.h"
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <termios.h>
static int step, calls, writes, fail_mouse, invalid, rgb16, release_outside;
static long offset;
static int claims, releases, key_quit, claim_fails, restored;
int mock_tcgetattr(int fd, struct termios *t) { assert(fd == 0); memset(t, 0, sizeof(*t)); t->c_lflag = ICANON | ECHO; return 0; }
int mock_tcsetattr(int fd, int action, const struct termios *t) { assert(fd == 0); restored = (t->c_lflag & (ICANON | ECHO)) == (ICANON | ECHO); return 0; }
ssize_t mock_read(int fd, void *buf, size_t n) { assert(fd == 0 && n == 1); if (!key_quit) return 0; *(char *)buf = key_quit; key_quit = 0; return 1; }
static unsigned char frame[240 * 112 * 4];
int mock_open(const char *name, int flags, ...) { assert(!strcmp(name, "/dev/fb0")); return 20; }
int mock_close(int fd) { assert(fd == 20); return 0; }
int mock_usleep(unsigned int delay) { assert(delay == 20000); return 0; }
off_t mock_lseek(int fd, off_t pos, int how) { assert(fd == 20 && how == 0); offset = pos; return pos; }
ssize_t mock_write(int fd, const void *buf, size_t n) {
    assert(fd == 20 && offset >= 0 && (size_t)offset + n <= sizeof(frame));
    memcpy(frame + offset, buf, n); writes++; return n;
}
long _onyx_ioctl(int fd, long op, long arg) {
    struct ow_screen *s = (struct ow_screen *)arg;
    assert(fd == 20);
    if (op == OW_FB_CLAIM_GRAPHICS) { claims++; return claim_fails ? -1 : 0; }
    if (op == OW_FB_RELEASE_GRAPHICS) { releases++; return 0; }
    assert(op == 0x4600);
    s->width = 240; s->height = 112; s->bpp = invalid ? 24 : rgb16 ? 16 : 32;
    s->pitch = rgb16 ? 484 : 960; s->size = s->pitch * s->height; return 0;
}
long _onyx_syscall1(long op, long arg) {
    unsigned char *p = (unsigned char *)arg;
    unsigned short *xy = (unsigned short *)arg;
    assert(op == 86);
    if (fail_mouse) return -1;
    xy[0] = release_outside && step == 2 ? 130 : 10; xy[1] = 70;
    /* Held button must fire only once, on release. */
    p[4] = step < 2 ? 1 : step == 2 ? 0 : 2;
    step++; return 0;
}
static int fetch(void *ctx, struct dashboard_sample *s) {
    (void)ctx; s->energy = 50; strcpy(s->machine, "MOCK TEST"); return 1;
}
static int action(void *ctx, int id, unsigned long request) {
    (void)ctx; assert(id == 1 && request == 1); calls++; return 1;
}
static int enqueue(void *ctx, unsigned long id, int action, unsigned long deadline) {
    (void)ctx; assert(id && action == 1 && deadline == 100); return 1;
}
static void session_test(void) {
    struct ow_session s;
    struct ow_transport t = {0, enqueue};
    unsigned long id;
    int i;
    ow_session_init(&s, &t);
    assert(!ow_session_send(&s, 1, 1, 100));
    ow_session_online(&s, 1);
    id = ow_session_send(&s, 1, 1, 100);
    assert(id && ow_session_find(&s, id)->state == OW_QUEUED);
    assert(ow_session_result(&s, id, OW_ACCEPTED));
    assert(ow_session_result(&s, id, OW_RUNNING));
    assert(!ow_session_result(&s, id, OW_ACCEPTED));
    assert(ow_session_result(&s, id, OW_COMPLETED));
    assert(!ow_session_result(&s, id, OW_FAILED));
    for (i = 0; i < OW_REQUESTS; i++) assert(ow_session_send(&s, 1, 1, 100));
    assert(!ow_session_send(&s, 1, 1, 100));
    ow_session_tick(&s, 100);
    assert(ow_session_find(&s, id + 1)->state == OW_TIMED_OUT);
    assert(!ow_session_result(&s, id + 1, OW_COMPLETED));
    id = ow_session_send(&s, 1, 1, 100);
    ow_session_online(&s, 0);
    assert(ow_session_find(&s, id)->state == OW_DISCONNECTED);
    ow_session_online(&s, 1);
    assert(!ow_session_result(&s, id, OW_COMPLETED));
    s.next_id = ~0UL;
    assert(ow_session_send(&s, 1, 1, 100) == ~0UL);
    assert(!ow_session_send(&s, 1, 1, 100));
}
int main(void) {
    session_test();
    struct ow_view v, before;
    struct ow_screen s = {1280, 720, 32, 5120, 3686400};
    struct dashboard_backend b = {0, fetch, action};
    int x, y;
    assert(ow_screen_valid(&s));
    assert(!ow_screen_pointer(&s, 0, 0, &x, &y));
    assert(ow_screen_pointer(&s, 40, 80, &x, &y) && x == 0 && y == 0);
    assert(ow_screen_pointer(&s, 1239, 639, &x, &y) && x == 239 && y == 111);
    assert(!ow_screen_pointer(&s, 1240, 640, &x, &y));
    assert(ow_screen_rgb(0xf800) == 0xff0000 && ow_screen_rgb(0xffff) == 0xffffff);
    s.pitch = 1; assert(!ow_screen_valid(&s));
    dashboard_init(&v); before = v;
    assert(!ow_panel_load(&v, "label 0 0 1 1 0 0 |ok\nBAD"));
    assert(!memcmp(&v, &before, sizeof(v)));
    assert(!ow_panel_load(&v, "label 999999999999999 0 1 1 0 0 |bad"));
    assert(!ow_panel_load(&v, "label 0 0 1 1 0 0 |bad\t"));
    assert(ow_panel_load(&v, "button 0 0 20 20 1 3 |CUSTOM\n"));
    assert(v.count == 1 && ow_pointer(&v, 1, 1) == 3);
    dashboard_init(&v); dashboard_refresh(&v, &b, 1);
    assert(dashboard_device(&v, &b));
    assert(calls == 1 && writes == 224 && frame[70 * 960 + 10 * 4] == 255);
    rgb16 = 1; step = 0; memset(frame, 0xa5, sizeof(frame));
    assert(dashboard_device(&v, &b));
    assert(calls == 2 && frame[70 * 484 + 20] == 255 && frame[480] == 0xa5);
    step = 0; release_outside = 1;
    assert(dashboard_device(&v, &b)); assert(calls == 2);
    invalid = 1; assert(!dashboard_device(&v, &b));
    invalid = 0; fail_mouse = 1; assert(!dashboard_device(&v, &b));
    assert(claims == releases && restored);
    fail_mouse = 0; step = 0; key_quit = 'q';
    assert(dashboard_device(&v, &b) && restored);
    step = 0; key_quit = 27;
    assert(dashboard_device(&v, &b) && restored);
    assert(claims == releases);
    claim_fails = 1;
    assert(!dashboard_device(&v, &b));
    assert(claims == releases + 1);
    dashboard_local(&v, &b, 1);
    assert(dashboard_refresh(&v, &b, 1));
    assert(!strcmp(v.widgets[0].text, "ONYX TABLET / LOCAL"));
    assert(dashboard_click(&v, &b, 10, 70, 1) == 2);
    assert(dashboard_refresh(&v, &b, 1) && v.widgets[3].value == 1);
    return 0;
}
