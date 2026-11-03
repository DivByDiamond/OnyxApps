#include "../dashboard.h"
#include "../graphics/raster.h"
#include <stdio.h>
#include <string.h>
static unsigned short pixels[240 * 112];
int demo_snapshot(struct ow_view *v, const char *path) {
    struct ow_raster r;
    FILE *f;
    int i;
    if (!ow_raster_init(&r, pixels, 240, 112, 240, demo_glyph)) return 0;
    memset(pixels, 0, sizeof(pixels));
    ow_raster_paint(v, &r);
    f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n240 112\n255\n");
    for (i = 0; i < 240 * 112; i++) {
        unsigned short c = pixels[i];
        fputc(((c >> 11) & 31) * 255 / 31, f);
        fputc(((c >> 5) & 63) * 255 / 63, f);
        fputc((c & 31) * 255 / 31, f);
    }
    i = ferror(f);
    if (fclose(f)) return 0;
    return !i;
}
