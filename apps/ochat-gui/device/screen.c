/* screen.c - framebuffer, mouse and keyboard loop for ochat-gui (OnyxOS only).
 * Reuses the otablet screen geometry (centred integer scaling) and the same
 * mouse/framebuffer ABI as otablet/device/frontend.c. */
#include "../gui/gui.h"
#include "device/display.h"
#include "link/link.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "../../../../OnyxCompiller/include/sys/syscalls.h"

/* Kernel repr(C) MouseEvent is six bytes; reserve eight for ABI compatibility. */
struct pointer { unsigned short x, y; unsigned char buttons, pad; unsigned short reserved; };
static unsigned short pixels[GUI_W * GUI_H], shown[GUI_W * GUI_H];
static unsigned char row[4096 * 4];
static int colmap[4096], rowmap[4096];
static struct termios saved_tio;
static int raw_on;

static void keyboard(int on) {
    struct termios t;
    if (on && !raw_on) {
        if (tcgetattr(0, &saved_tio) != 0) return;
        t = saved_tio;
        cfmakeraw_apply(&t);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(0, 0, &t);
        raw_on = 1;
    } else if (!on && raw_on) { tcsetattr(0, 0, &saved_tio); raw_on = 0; }
}

static int present(int fd, const struct ow_screen *s, int mx, int my) {
    int x, y, k, bytes = s->bpp / 8;
    for (y = 0; y < (int)s->height; y++) {
        unsigned int c;
        memset(row, 0, sizeof(row));
        for (x = 0; x < (int)s->width; x++) {
            c = 0;
            if (rowmap[y] >= 0 && colmap[x] >= 0) c = pixels[rowmap[y] * GUI_W + colmap[x]];
            if (x >= mx && x < mx + 5 && y >= my && y < my + 5) c = 65535;
            if (s->bpp == 32) c = ow_screen_rgb(c);
            for (k = 0; k < bytes; k++) row[x * bytes + k] = (unsigned char)(c >> (k * 8));
        }
        if (lseek(fd, y * s->pitch, 0) < 0 ||
            write(fd, row, s->width * bytes) != (long)(s->width * bytes)) return 0;
    }
    return 1;
}

/* Returns 0 on a clean exit, -1 if the framebuffer or mouse failed. */
int screen_run(struct gui *g) {
    struct ow_screen screen;
    struct pointer p;
    int fd, x, y, i, down = 0, armed = 0, lastx = -1, lasty = -1, dirty = 1, rc = 0;
    unsigned char key;
    fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return -1;
    if (_onyx_ioctl(fd, 0x4600, (long)&screen) < 0 || !ow_screen_valid(&screen)) { close(fd); return -1; }
    for (i = 0; i < (int)screen.width; i++)
        colmap[i] = ow_screen_pointer(&screen, i, (int)screen.height / 2, &x, &y) ? x : -1;
    for (i = 0; i < (int)screen.height; i++)
        rowmap[i] = ow_screen_pointer(&screen, (int)screen.width / 2, i, &x, &y) ? y : -1;
    keyboard(1);
    for (;;) {
        long now = ochat_now_ms();
        int quit = 0;
        if (_onyx_syscall1(SYS_mouse_read, (long)&p) < 0) { rc = -1; break; }
        if (p.buttons & 2) break;                               /* right click exits */
        if ((p.buttons & 1) && !down)
            armed = ow_screen_pointer(&screen, p.x, p.y, &x, &y) ? ow_pointer(&g->view, x, y) : 0;
        if (!(p.buttons & 1) && down) {
            if (armed && ow_screen_pointer(&screen, p.x, p.y, &x, &y) &&
                ow_pointer(&g->view, x, y) == armed) { gui_click(g, x, y, now); dirty = 1; }
            armed = 0;
        }
        down = p.buttons & 1;
        while (read(0, &key, 1) == 1) { quit |= gui_key(g, key, now); dirty = 1; }
        if (quit) break;
        gui_tick(g, now);
        gui_draw(g, pixels);
        if (memcmp(pixels, shown, sizeof(pixels))) { memcpy(shown, pixels, sizeof(pixels)); dirty = 1; }
        if (dirty || lastx != p.x || lasty != p.y) {
            if (!present(fd, &screen, p.x, p.y)) { rc = -1; break; }
            dirty = 0; lastx = p.x; lasty = p.y;
        }
        usleep(20000);
    }
    keyboard(0);
    close(fd);
    return rc;
}
