#include "raster.h"
int ow_raster_init(struct ow_raster *r, unsigned short *pixels, int w, int h,
                   int stride, unsigned int (*glyph)(unsigned char, int)) {
    if (!r || !pixels || !glyph || w < 1 || h < 1 || w > OW_EXTENT ||
        h > OW_EXTENT || stride < w || stride > OW_EXTENT) return 0;
    r->pixels = pixels; r->width = w; r->height = h;
    r->stride = stride; r->glyph = glyph;
    return 1;
}
static void pixel(struct ow_raster *r, int x, int y, unsigned short c) {
    if (x >= 0 && y >= 0 && x < r->width && y < r->height)
        r->pixels[y * r->stride + x] = c;
}
static void fill(void *ctx, int x, int y, int w, int h, unsigned short c) {
    struct ow_raster *r = ctx;
    int a, b;
    for (b = y; b < y + h; b++)
        for (a = x; a < x + w; a++) pixel(r, a, b, c);
}
static void text(void *ctx, int x, int y, const char *s,
                 int cx, int cy, int cw, int ch, unsigned short c) {
    struct ow_raster *r = ctx;
    int i, a, b;
    for (i = 0; s[i]; i++) {
        for (b = 0; b < 7; b++) {
            unsigned int bits = r->glyph((unsigned char)s[i], b);
            for (a = 0; a < 5; a++) {
                int px = x + i * 6 + a, py = y + b;
                if ((bits & (1u << (4 - a))) && px >= cx && py >= cy &&
                    px < cx + cw && py < cy + ch) pixel(r, px, py, c);
            }
        }
    }
}
void ow_raster_paint(const struct ow_view *v, struct ow_raster *r) {
    struct ow_painter p;
    if (!r || !r->pixels || !r->glyph) return;
    p.fill = fill; p.text = text;
    ow_paint(v, &p, r);
}
