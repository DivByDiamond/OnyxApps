#ifndef OW_RASTER_H
#define OW_RASTER_H
#include "../core/widget.h"
/* Pixels must contain at least stride*height elements; caller owns storage.
 * glyph returns the 5 low bits of one row of a 5x7 glyph, or 0 for blank.
 * No font, framebuffer device, allocator or platform ABI is baked in. */
struct ow_raster {
    unsigned short *pixels;
    int width, height, stride;
    unsigned int (*glyph)(unsigned char ch, int row);
};
int ow_raster_init(struct ow_raster *, unsigned short *, int, int, int,
                   unsigned int (*)(unsigned char, int));
void ow_raster_paint(const struct ow_view *, struct ow_raster *);
#endif
