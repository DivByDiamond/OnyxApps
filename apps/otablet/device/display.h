#ifndef OW_DISPLAY_H
#define OW_DISPLAY_H
#include "../dashboard.h"
struct ow_screen { unsigned int width, height, bpp, pitch, size; };
int ow_screen_valid(const struct ow_screen *);
int ow_screen_pointer(const struct ow_screen *, int, int, int *, int *);
unsigned int ow_screen_rgb(unsigned short);
int dashboard_device(struct ow_view *, const struct dashboard_backend *);
#endif
