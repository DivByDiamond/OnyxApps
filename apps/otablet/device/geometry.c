#include "display.h"
int ow_screen_valid(const struct ow_screen *s) {
    unsigned int bytes;
    if (!s || s->width < 240 || s->height < 112 ||
        s->width > 4096 || s->height > 4096 || (s->bpp != 16 && s->bpp != 32)) return 0;
    bytes = s->bpp / 8;
    return s->pitch >= s->width * bytes && s->pitch <= 65536 &&
        s->size >= s->pitch * s->height && s->size <= 268435456;
}
int ow_screen_pointer(const struct ow_screen *s, int x, int y, int *px, int *py) {
    int scale, left, top;
    if (!ow_screen_valid(s) || !px || !py) return 0;
    scale = s->width / 240;
    if (scale > (int)s->height / 112) scale = s->height / 112;
    left = ((int)s->width - 240 * scale) / 2;
    top = ((int)s->height - 112 * scale) / 2;
    if (x < left || y < top || x >= left + 240 * scale || y >= top + 112 * scale) return 0;
    *px = (x - left) / scale; *py = (y - top) / scale;
    return 1;
}
unsigned int ow_screen_rgb(unsigned short c) {
    return (((c >> 11) & 31) * 255 / 31 << 16) |
        (((c >> 5) & 63) * 255 / 63 << 8) | ((c & 31) * 255 / 31);
}
