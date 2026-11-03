#include "display.h"
#include "../graphics/raster.h"
#include <stdio.h>
#include <string.h>
#ifdef __onyx__
#include <unistd.h>
#include <fcntl.h>
#include "../../../../OnyxCompiller/include/sys/syscalls.h"
/* Kernel repr(C) MouseEvent is six bytes; reserve eight for ABI compatibility. */
struct pointer { unsigned short x, y; unsigned char buttons, pad; unsigned short reserved; };
static unsigned short pixels[240 * 112];
static unsigned char row[4096 * 4];
static int present(int fd, const struct ow_screen *s, struct ow_view *v, int mx, int my) {
    struct ow_raster r;
    int x, y, px, py, k, bytes = s->bpp / 8;
    unsigned int c;
    memset(pixels, 0, sizeof(pixels));
    ow_raster_init(&r, pixels, 240, 112, 240, demo_glyph);
    ow_raster_paint(v, &r);
    for (y = 0; y < (int)s->height; y++) {
        memset(row, 0, sizeof(row));
        for (x = 0; x < (int)s->width; x++) {
            c = 0;
            if (ow_screen_pointer(s, x, y, &px, &py)) c = pixels[py * 240 + px];
            if (x >= mx && x < mx + 5 && y >= my && y < my + 5) c = 65535;
            if (s->bpp == 32) c = ow_screen_rgb(c);
            for (k = 0; k < bytes; k++) row[x * bytes + k] = c >> (k * 8);
        }
        if (lseek(fd, y * s->pitch, 0) < 0 ||
            write(fd, row, s->width * bytes) != (long)(s->width * bytes)) return 0;
    }
    return 1;
}
int dashboard_device(struct ow_view *v, const struct dashboard_backend *b) {
    struct ow_screen screen;
    struct pointer p;
    unsigned long request = 1;
    int fd, x, y, down = 0, armed = 0, lastx = -1, lasty = -1, dirty = 1;
    fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return 0;
    if (_onyx_ioctl(fd, 0x4600, (long)&screen) < 0 || !ow_screen_valid(&screen)) {
        close(fd); return 0;
    }
    /* Right mouse button exits. Never polls stdin or fabricates network updates. */
    for (;;) {
        if (_onyx_syscall1(SYS_mouse_read, (long)&p) < 0) break;
        if (p.buttons & 2) { close(fd); return 1; }
        if ((p.buttons & 1) && !down) {
            armed = ow_screen_pointer(&screen, p.x, p.y, &x, &y) ? ow_pointer(v, x, y) : 0;
        }
        if (!(p.buttons & 1) && down) {
            if (armed && ow_screen_pointer(&screen, p.x, p.y, &x, &y) && ow_pointer(v, x, y) == armed) {
                dashboard_click(v, b, x, y, request++);
                dashboard_refresh(v, b, request);
                dirty = 1;
            }
            armed = 0;
        }
        down = p.buttons & 1;
        if (dirty || lastx != p.x || lasty != p.y) {
            if (!present(fd, &screen, v, p.x, p.y)) break;
            dirty = 0; lastx = p.x; lasty = p.y;
        }
        usleep(20000);
    }
    close(fd); return 0;
}
#else
int dashboard_device(struct ow_view *v, const struct dashboard_backend *b) {
    (void)v; (void)b;
    return 0;
}
#endif
