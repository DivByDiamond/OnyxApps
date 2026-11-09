#ifndef OW_DISPLAY_H
#define OW_DISPLAY_H
#include "../dashboard.h"
struct ow_screen { unsigned int width, height, bpp, pitch, size; };
int ow_screen_valid(const struct ow_screen *);
int ow_screen_pointer(const struct ow_screen *, int, int, int *, int *);
unsigned int ow_screen_rgb(unsigned short);
#define OW_FB_CLAIM_GRAPHICS 0x46f0
#define OW_FB_RELEASE_GRAPHICS 0x46f1
void dashboard_local(struct ow_view *, struct dashboard_backend *, int);
int dashboard_device(struct ow_view *, const struct dashboard_backend *);
#endif
